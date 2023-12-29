/*
 * Copyright 2022-2023 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include "internal/quic_demux.h"
#include "internal/quic_wire_pkt.h"
#include "internal/common.h"
#include <openssl/lhash.h>
#include <openssl/err.h>

#define URXE_DEMUX_STATE_FREE       0 /* on urx_free list */
#define URXE_DEMUX_STATE_PENDING    1 /* on urx_pending list */
#define URXE_DEMUX_STATE_ISSUED     2 /* on neither list */

#define DEMUX_MAX_MSGS_PER_CALL    32

#define DEMUX_DEFAULT_MTU        1500

/* Structure used to track a given connection ID. */
typedef struct quic_demux_conn_st QUIC_DEMUX_CONN;

struct quic_demux_conn_st {
    QUIC_DEMUX_CONN                 *next; /* used when unregistering only */
    QUIC_CONN_ID                    dst_conn_id;
    ossl_quic_demux_cb_fn           *cb;
    void                            *cb_arg;
};

DEFINE_LHASH_OF_EX(QUIC_DEMUX_CONN);

static unsigned long demux_conn_hash(const QUIC_DEMUX_CONN *conn)
{
    size_t i;
    unsigned long v = 0;

    assert(conn->dst_conn_id.id_len <= QUIC_MAX_CONN_ID_LEN);

    for (i = 0; i < conn->dst_conn_id.id_len; ++i)
        v ^= ((unsigned long)conn->dst_conn_id.id[i])
             << ((i * 8) % (sizeof(unsigned long) * 8));

    return v;
}

static int demux_conn_cmp(const QUIC_DEMUX_CONN *a, const QUIC_DEMUX_CONN *b)
{
    return !ossl_quic_conn_id_eq(&a->dst_conn_id, &b->dst_conn_id);
}

struct quic_demux_st {
    /* The underlying transport BIO with datagram semantics. */
    BIO                        *net_bio;

    /*
     * QUIC short packets do not contain the length of the connection ID field,
     * therefore it must be known contextually. The demuxer requires connection
     * IDs of the same length to be used for all incoming packets.
     */
    size_t                      short_conn_id_len;

    /*
     * Our current understanding of the upper bound on an incoming datagram size
     * in bytes.
     */
    size_t                      mtu;

    /* Time retrieval callback. */
    OSSL_TIME                 (*now)(void *arg);
    void                       *now_arg;

    /* Hashtable mapping connection IDs to QUIC_DEMUX_CONN structures. */
    LHASH_OF(QUIC_DEMUX_CONN)  *conns_by_id;

    /* The default packet handler, if any. */
    ossl_quic_demux_cb_fn      *default_cb;
    void                       *default_cb_arg;

    /* The stateless reset token checker handler, if any. */
    ossl_quic_stateless_reset_cb_fn *reset_token_cb;
    void                            *reset_token_cb_arg;

    /*
     * List of URXEs which are not currently in use (i.e., not filled with
     * unconsumed data). These are moved to the pending list as they are filled.
     */
    QUIC_URXE_LIST              urx_free;

    /*
     * List of URXEs which are filled with received encrypted data. These are
     * removed from this list as we invoke the callbacks for each of them. They
     * are then not on any list managed by us; we forget about them until our
     * user calls ossl_quic_demux_release_urxe to return the URXE to us, at
     * which point we add it to the free list.
     */
    QUIC_URXE_LIST              urx_pending;

    /* Whether to use local address support. */
    char                        use_local_addr;
};

QUIC_DEMUX *ossl_quic_demux_new(BIO *net_bio,
                                size_t short_conn_id_len,
                                OSSL_TIME (*now)(void *arg),
                                void *now_arg)
{
    QUIC_DEMUX *demux;

    demux = OPENSSL_zalloc(sizeof(QUIC_DEMUX));
    if (demux == NULL)
        return NULL;

    demux->net_bio                  = net_bio;
    demux->short_conn_id_len        = short_conn_id_len;
    /* We update this if possible when we get a BIO. */
    demux->mtu                      = DEMUX_DEFAULT_MTU;
    demux->now                      = now;
    demux->now_arg                  = now_arg;

    demux->conns_by_id
        = lh_QUIC_DEMUX_CONN_new(demux_conn_hash, demux_conn_cmp);
    if (demux->conns_by_id == NULL) {
        OPENSSL_free(demux);
        return NULL;
    }

    if (net_bio != NULL
        && BIO_dgram_get_local_addr_cap(net_bio)
        && BIO_dgram_set_local_addr_enable(net_bio, 1))
        demux->use_local_addr = 1;

    return demux;
}

static void demux_free_conn_it(QUIC_DEMUX_CONN *conn, void *arg)
{
    OPENSSL_free(conn);
}

static void demux_free_urxl(QUIC_URXE_LIST *l)
{
    QUIC_URXE *e, *enext;

    for (e = ossl_list_urxe_head(l); e != NULL; e = enext) {
        enext = ossl_list_urxe_next(e);
        ossl_list_urxe_remove(l, e);
        OPENSSL_free(e);
    }
}

void ossl_quic_demux_free(QUIC_DEMUX *demux)
{
    if (demux == NULL)
        return;

    /* Free all connection structures. */
    lh_QUIC_DEMUX_CONN_doall_arg(demux->conns_by_id, demux_free_conn_it, NULL);
    lh_QUIC_DEMUX_CONN_free(demux->conns_by_id);

    /* Free all URXEs we are holding. */
    demux_free_urxl(&demux->urx_free);
    demux_free_urxl(&demux->urx_pending);

    OPENSSL_free(demux);
}

void ossl_quic_demux_set_bio(QUIC_DEMUX *demux, BIO *net_bio)
{
    unsigned int mtu;

    demux->net_bio = net_bio;

    if (net_bio != NULL) {
        /*
         * Try to determine our MTU if possible. The BIO is not required to
         * support this, in which case we remain at the last known MTU, or our
         * initial default.
         */
        mtu = BIO_dgram_get_mtu(net_bio);
        if (mtu >= QUIC_MIN_INITIAL_DGRAM_LEN)
            ossl_quic_demux_set_mtu(demux, mtu); /* best effort */
    }
}

int ossl_quic_demux_set_mtu(QUIC_DEMUX *demux, unsigned int mtu)
{
    if (mtu < QUIC_MIN_INITIAL_DGRAM_LEN)
        return 0;

    demux->mtu = mtu;
    return 1;
}

static QUIC_DEMUX_CONN *demux_get_by_conn_id(QUIC_DEMUX *demux,
                                             const QUIC_CONN_ID *dst_conn_id)
{
    QUIC_DEMUX_CONN key;

    if (dst_conn_id->id_len > QUIC_MAX_CONN_ID_LEN)
        return NULL;

    key.dst_conn_id = *dst_conn_id;
    return lh_QUIC_DEMUX_CONN_retrieve(demux->conns_by_id, &key);
}

int ossl_quic_demux_register(QUIC_DEMUX *demux,
                             const QUIC_CONN_ID *dst_conn_id,
                             ossl_quic_demux_cb_fn *cb, void *cb_arg)
{
    QUIC_DEMUX_CONN *conn;

    if (dst_conn_id == NULL
        || dst_conn_id->id_len > QUIC_MAX_CONN_ID_LEN
        || cb == NULL)
        return 0;

    /* Ensure not already registered. */
    if (demux_get_by_conn_id(demux, dst_conn_id) != NULL)
        /* Handler already registered with this connection ID. */
        return 0;

    conn = OPENSSL_zalloc(sizeof(QUIC_DEMUX_CONN));
    if (conn == NULL)
        return 0;

    conn->dst_conn_id   = *dst_conn_id;
    conn->cb            = cb;
    conn->cb_arg        = cb_arg;

    lh_QUIC_DEMUX_CONN_insert(demux->conns_by_id, conn);
    return 1;
}

static void demux_unregister(QUIC_DEMUX *demux,
                             QUIC_DEMUX_CONN *conn)
{
    lh_QUIC_DEMUX_CONN_delete(demux->conns_by_id, conn);
    OPENSSL_free(conn);
}

int ossl_quic_demux_unregister(QUIC_DEMUX *demux,
                               const QUIC_CONN_ID *dst_conn_id)
{
    QUIC_DEMUX_CONN *conn;

    if (dst_conn_id == NULL
        || dst_conn_id->id_len > QUIC_MAX_CONN_ID_LEN)
        return 0;

    conn = demux_get_by_conn_id(demux, dst_conn_id);
    if (conn == NULL)
        return 0;

    demux_unregister(demux, conn);
    return 1;
}

struct unreg_arg {
    ossl_quic_demux_cb_fn *cb;
    void *cb_arg;
    QUIC_DEMUX_CONN *head;
};

static void demux_unregister_by_cb(QUIC_DEMUX_CONN *conn, void *arg_)
{
    struct unreg_arg *arg = arg_;

    if (conn->cb == arg->cb && conn->cb_arg == arg->cb_arg) {
        conn->next = arg->head;
        arg->head = conn;
    }
}

void ossl_quic_demux_unregister_by_cb(QUIC_DEMUX *demux,
                                      ossl_quic_demux_cb_fn *cb,
                                      void *cb_arg)
{
    QUIC_DEMUX_CONN *conn, *cnext;
    struct unreg_arg arg = {0};
    arg.cb      = cb;
    arg.cb_arg  = cb_arg;

    lh_QUIC_DEMUX_CONN_doall_arg(demux->conns_by_id,
                                 demux_unregister_by_cb, &arg);

    for (conn = arg.head; conn != NULL; conn = cnext) {
        cnext = conn->next;
        demux_unregister(demux, conn);
    }
}

void ossl_quic_demux_set_default_handler(QUIC_DEMUX *demux,
                                         ossl_quic_demux_cb_fn *cb,
                                         void *cb_arg)
{
    demux->default_cb       = cb;
    demux->default_cb_arg   = cb_arg;
}

void ossl_quic_demux_set_stateless_reset_handler(
        QUIC_DEMUX *demux,
        ossl_quic_stateless_reset_cb_fn *cb, void *cb_arg)
{
    demux->reset_token_cb       = cb;
    demux->reset_token_cb_arg   = cb_arg;
}

static QUIC_URXE *demux_alloc_urxe(size_t alloc_len)
{
    QUIC_URXE *e;

    if (alloc_len >= SIZE_MAX - sizeof(QUIC_URXE))
        return NULL;

    e = OPENSSL_malloc(sizeof(QUIC_URXE) + alloc_len);
    if (e == NULL)
        return NULL;

    ossl_list_urxe_init_elem(e);
    e->alloc_len   = alloc_len;
    e->data_len    = 0;
    return e;
}

static QUIC_URXE *demux_resize_urxe(QUIC_DEMUX *demux, QUIC_URXE *e,
                                    size_t new_alloc_len)
{
    QUIC_URXE *e2, *prev;

    if (!ossl_assert(e->demux_state == URXE_DEMUX_STATE_FREE))
        /* Never attempt to resize a URXE which is not on the free list. */
        return NULL;

    prev = ossl_list_urxe_prev(e);
    ossl_list_urxe_remove(&demux->urx_free, e);

    e2 = OPENSSL_realloc(e, sizeof(QUIC_URXE) + new_alloc_len);
    if (e2 == NULL) {
        /* Failed to resize, abort. */
        if (prev == NULL)
            ossl_list_urxe_insert_head(&demux->urx_free, e);
        else
            ossl_list_urxe_insert_after(&demux->urx_free, prev, e);

        return NULL;
    }

    if (prev == NULL)
        ossl_list_urxe_insert_head(&demux->urx_free, e2);
    else
        ossl_list_urxe_insert_after(&demux->urx_free, prev, e2);

    e2->alloc_len = new_alloc_len;
    return e2;
}

static QUIC_URXE *demux_reserve_urxe(QUIC_DEMUX *demux, QUIC_URXE *e,
                                     size_t alloc_len)
{
    return e->alloc_len < alloc_len ? demux_resize_urxe(demux, e, alloc_len) : e;
}

static int demux_ensure_free_urxe(QUIC_DEMUX *demux, size_t min_num_free)
{
    QUIC_URXE *e;

    while (ossl_list_urxe_num(&demux->urx_free) < min_num_free) {
        e = demux_alloc_urxe(demux->mtu);
        if (e == NULL)
            return 0;

        ossl_list_urxe_insert_tail(&demux->urx_free, e);
        e->demux_state = URXE_DEMUX_STATE_FREE;
    }

    return 1;
}

/*
 * Receive datagrams from network, placing them into URXEs.
 *
 * Returns 1 on success or 0 on failure.
 *
 * Precondition: at least one URXE is free
 * Precondition: there are no pending URXEs
 */
static int demux_recv(QUIC_DEMUX *demux)
{
    BIO_MSG msg[DEMUX_MAX_MSGS_PER_CALL];
    size_t rd, i;
    QUIC_URXE *urxe = ossl_list_urxe_head(&demux->urx_free), *unext;
    OSSL_TIME now;

    /* This should never be called when we have any pending URXE. */
    assert(ossl_list_urxe_head(&demux->urx_pending) == NULL);
    assert(urxe->demux_state == URXE_DEMUX_STATE_FREE);

    if (demux->net_bio == NULL)
        /*
         * If no BIO is plugged in, treat this as no datagram being available.
         */
        return QUIC_DEMUX_PUMP_RES_TRANSIENT_FAIL;

    /*
     * Opportunistically receive as many messages as possible in a single
     * syscall, determined by how many free URXEs are available.
     */
    for (i = 0; i < (ossl_ssize_t)OSSL_NELEM(msg);
            ++i, urxe = ossl_list_urxe_next(urxe)) {
        if (urxe == NULL) {
            /* We need at least one URXE to receive into. */
            if (!ossl_assert(i > 0))
                return QUIC_DEMUX_PUMP_RES_PERMANENT_FAIL;

            break;
        }

        /* Ensure the URXE is big enough. */
        urxe = demux_reserve_urxe(demux, urxe, demux->mtu);
        if (urxe == NULL)
            /* Allocation error, fail. */
            return QUIC_DEMUX_PUMP_RES_PERMANENT_FAIL;

        /* Ensure we zero any fields added to BIO_MSG at a later date. */
        memset(&msg[i], 0, sizeof(BIO_MSG));
        msg[i].data     = ossl_quic_urxe_data(urxe);
        msg[i].data_len = urxe->alloc_len;
        msg[i].peer     = &urxe->peer;
        BIO_ADDR_clear(&urxe->peer);
        if (demux->use_local_addr)
            msg[i].local = &urxe->local;
        else
            BIO_ADDR_clear(&urxe->local);
    }

    ERR_set_mark();
    if (!BIO_recvmmsg(demux->net_bio, msg, sizeof(BIO_MSG), i, 0, &rd)) {
        if (BIO_err_is_non_fatal(ERR_peek_last_error())) {
            /* Transient error, clear the error and stop. */
            ERR_pop_to_mark();
            return QUIC_DEMUX_PUMP_RES_TRANSIENT_FAIL;
        } else {
            /* Non-transient error, do not clear the error. */
            ERR_clear_last_mark();
            return QUIC_DEMUX_PUMP_RES_PERMANENT_FAIL;
        }
    }

    ERR_clear_last_mark();
    now = demux->now != NULL ? demux->now(demux->now_arg) : ossl_time_zero();

    urxe = ossl_list_urxe_head(&demux->urx_free);
    for (i = 0; i < rd; ++i, urxe = unext) {
        unext = ossl_list_urxe_next(urxe);
        /* Set URXE with actual length of received datagram. */
        urxe->data_len      = msg[i].data_len;
        /* Time we received datagram. */
        urxe->time          = now;
        /* Move from free list to pending list. */
        ossl_list_urxe_remove(&demux->urx_free, urxe);
        ossl_list_urxe_insert_tail(&demux->urx_pending, urxe);
        urxe->demux_state = URXE_DEMUX_STATE_PENDING;
    }

    return QUIC_DEMUX_PUMP_RES_OK;
}

/* Extract destination connection ID from the first packet in a datagram. */
static int demux_identify_conn_id(QUIC_DEMUX *demux,
                                  QUIC_URXE *e,
                                  QUIC_CONN_ID *dst_conn_id)
{
    return ossl_quic_wire_get_pkt_hdr_dst_conn_id(ossl_quic_urxe_data(e),
                                                  e->data_len,
                                                  demux->short_conn_id_len,
                                                  dst_conn_id);
}

/* Identify the connection structure corresponding to a given URXE. */
static QUIC_DEMUX_CONN *demux_identify_conn(QUIC_DEMUX *demux, QUIC_URXE *e)
{
    QUIC_CONN_ID dst_conn_id;

    if (!demux_identify_conn_id(demux, e, &dst_conn_id))
        /*
         * Datagram is so badly malformed we can't get the DCID from the first
         * packet in it, so just give up.
         */
        return NULL;

    return demux_get_by_conn_id(demux, &dst_conn_id);
}

/*
 * Process a single pending URXE.
 * Returning 1 on success, 0 on failure and -1 on stateless reset.
 */
static int demux_process_pending_urxe(QUIC_DEMUX *demux, QUIC_URXE *e)
{
    QUIC_DEMUX_CONN *conn;
    int r;

    /* The next URXE we process should be at the head of the pending list. */
    if (!ossl_assert(e == ossl_list_urxe_head(&demux->urx_pending)))
        return 0;

    assert(e->demux_state == URXE_DEMUX_STATE_PENDING);

    /*
     * Check if the packet ends with a stateless reset token and if it does
     * skip it after dropping the connection.
     *
     * RFC 9000 s. 10.3.1 Detecting a Stateless Reset
     *      If the last 16 bytes of the datagram are identical in value to
     *      a stateless reset token, the endpoint MUST enter the draining
     *      period and not send any further packets on this connection.
     *
     * Returning a failure here causes the connection to enter the terminating
     * state which achieves the desired outcome.
     *
     * TODO(QUIC FUTURE): only try to match unparsable packets
     */
    if (demux->reset_token_cb != NULL) {
        r = demux->reset_token_cb(ossl_quic_urxe_data(e), e->data_len,
                                  demux->reset_token_cb_arg);
        if (r > 0)      /* Received a stateless reset */
            return -1;
        if (r < 0)      /* Error during stateless reset detection */
            return 0;
    }

    conn = demux_identify_conn(demux, e);
    if (conn == NULL) {
        /*
         * We could not identify a connection. If we have a default packet
         * handler, pass it to the handler. Otherwise, we will never be able to
         * process this datagram, so get rid of it.
         */
        ossl_list_urxe_remove(&demux->urx_pending, e);
        if (demux->default_cb != NULL) {
            /* Pass to default handler. */
            e->demux_state = URXE_DEMUX_STATE_ISSUED;
            demux->default_cb(e, demux->default_cb_arg);
        } else {
            /* Discard. */
            ossl_list_urxe_insert_tail(&demux->urx_free, e);
            e->demux_state = URXE_DEMUX_STATE_FREE;
        }
        return 1; /* keep processing pending URXEs */
    }

    /*
     * Remove from list and invoke callback. The URXE now belongs to the
     * callback. (QUIC_DEMUX_CONN never has non-NULL cb.)
     */
    ossl_list_urxe_remove(&demux->urx_pending, e);
    e->demux_state = URXE_DEMUX_STATE_ISSUED;
    conn->cb(e, conn->cb_arg);
    return 1;
}

/* Process pending URXEs to generate callbacks. */
static int demux_process_pending_urxl(QUIC_DEMUX *demux)
{
    QUIC_URXE *e;
    int ret;

    while ((e = ossl_list_urxe_head(&demux->urx_pending)) != NULL)
        if ((ret = demux_process_pending_urxe(demux, e)) <= 0)
            return ret;

    return 1;
}

/*
 * Drain the pending URXE list, processing any pending URXEs by making their
 * callbacks. If no URXEs are pending, a network read is attempted first.
 */
int ossl_quic_demux_pump(QUIC_DEMUX *demux)
{
    int ret;

    if (ossl_list_urxe_head(&demux->urx_pending) == NULL) {
        ret = demux_ensure_free_urxe(demux, DEMUX_MAX_MSGS_PER_CALL);
        if (ret != 1)
            return QUIC_DEMUX_PUMP_RES_PERMANENT_FAIL;

        ret = demux_recv(demux);
        if (ret != QUIC_DEMUX_PUMP_RES_OK)
            return ret;

        /*
         * If demux_recv returned successfully, we should always have something.
         */
        assert(ossl_list_urxe_head(&demux->urx_pending) != NULL);
    }

    if ((ret = demux_process_pending_urxl(demux)) <= 0)
        return ret == 0 ? QUIC_DEMUX_PUMP_RES_PERMANENT_FAIL
                        : QUIC_DEMUX_PUMP_RES_STATELESS_RESET;

    return QUIC_DEMUX_PUMP_RES_OK;
}

/* Artificially inject a packet into the demuxer for testing purposes. */
int ossl_quic_demux_inject(QUIC_DEMUX *demux,
                           const unsigned char *buf,
                           size_t buf_len,
                           const BIO_ADDR *peer,
                           const BIO_ADDR *local)
{
    int ret;
    QUIC_URXE *urxe;

    ret = demux_ensure_free_urxe(demux, 1);
    if (ret != 1)
        return 0;

    urxe = ossl_list_urxe_head(&demux->urx_free);

    assert(urxe->demux_state == URXE_DEMUX_STATE_FREE);

    urxe = demux_reserve_urxe(demux, urxe, buf_len);
    if (urxe == NULL)
        return 0;

    memcpy(ossl_quic_urxe_data(urxe), buf, buf_len);
    urxe->data_len = buf_len;

    if (peer != NULL)
        urxe->peer = *peer;
    else
        BIO_ADDR_clear(&urxe->peer);

    if (local != NULL)
        urxe->local = *local;
    else
        BIO_ADDR_clear(&urxe->local);

    urxe->time
        = demux->now != NULL ? demux->now(demux->now_arg) : ossl_time_zero();

    /* Move from free list to pending list. */
    ossl_list_urxe_remove(&demux->urx_free, urxe);
    ossl_list_urxe_insert_tail(&demux->urx_pending, urxe);
    urxe->demux_state = URXE_DEMUX_STATE_PENDING;

    return demux_process_pending_urxl(demux) > 0;
}

/* Called by our user to return a URXE to the free list. */
void ossl_quic_demux_release_urxe(QUIC_DEMUX *demux,
                                  QUIC_URXE *e)
{
    assert(ossl_list_urxe_prev(e) == NULL && ossl_list_urxe_next(e) == NULL);
    assert(e->demux_state == URXE_DEMUX_STATE_ISSUED);
    ossl_list_urxe_insert_tail(&demux->urx_free, e);
    e->demux_state = URXE_DEMUX_STATE_FREE;
}

void ossl_quic_demux_reinject_urxe(QUIC_DEMUX *demux,
                                   QUIC_URXE *e)
{
    assert(ossl_list_urxe_prev(e) == NULL && ossl_list_urxe_next(e) == NULL);
    assert(e->demux_state == URXE_DEMUX_STATE_ISSUED);
    ossl_list_urxe_insert_head(&demux->urx_pending, e);
    e->demux_state = URXE_DEMUX_STATE_PENDING;
}

int ossl_quic_demux_has_pending(const QUIC_DEMUX *demux)
{
    return ossl_list_urxe_head(&demux->urx_pending) != NULL;
}
