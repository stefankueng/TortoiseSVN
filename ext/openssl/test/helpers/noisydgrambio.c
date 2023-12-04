/*
 * Copyright 2023 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/bio.h>
#include "quictestlib.h"
#include "../testutil.h"

#define MSG_DATA_LEN_MAX    1472

struct noisy_dgram_st {
    uint64_t this_dgram;
    BIO_MSG msg;
    uint64_t reinject_dgram;
    int backoff;
};

static long noisy_dgram_ctrl(BIO *bio, int cmd, long num, void *ptr)
{
    long ret;
    BIO *next = BIO_next(bio);

    if (next == NULL)
        return 0;

    switch (cmd) {
    case BIO_CTRL_DUP:
        ret = 0L;
        break;
    case BIO_CTRL_NOISE_BACK_OFF: {
            struct noisy_dgram_st *data;

            data = BIO_get_data(bio);
            if (!TEST_ptr(data))
                return 0;
            data->backoff = 1;
            ret = 1;
            break;
        }
    default:
        ret = BIO_ctrl(next, cmd, num, ptr);
        break;
    }
    return ret;
}

static int noisy_dgram_sendmmsg(BIO *bio, BIO_MSG *msg, size_t stride,
                                size_t num_msg, uint64_t flags,
                                size_t *msgs_processed)
{
    BIO *next = BIO_next(bio);

    if (next == NULL)
        return 0;

    /*
     * We only introduce noise when receiving messages. We just pass this on
     * to the underlying BIO.
     */
    return BIO_sendmmsg(next, msg, stride, num_msg, flags, msgs_processed);
}

/* 1 in NOISE_RATE datagrams will be noisy. With a value of 5 that is 20% */
#define NOISE_RATE  5

/*
 * We have 3 different types of noise: drop, duplicate and delay
 * Each of these have equal probability.
 */
#define NOISE_TYPE_DROP      0
#define NOISE_TYPE_DUPLICATE 1
#define NOISE_TYPE_DELAY     2
#define NOISE_TYPE_BITFLIPS  3
#define NUM_NOISE_TYPES      4

/*
 * When a duplicate occurs we reinject the new datagram after up to
 * MAX_DGRAM_REINJECT datagrams have been sent. A reinject of 1 means that the
 * duplicate follows immediately after the original datagram. A reinject of 4
 * means that original datagram plus 3 other datagrams are sent before the
 * reinjected datagram is inserted.
 * This also controls when a delay (not a duplicate) occurs. In that case
 * we add 1 to the number because there is no point in skipping the current
 * datagram only to immediately reinject it in the next datagram.
 */
#define MAX_DGRAM_REINJECT 4

static void get_noise(int long_header, uint64_t *reinject, int *should_drop,
                      uint16_t *flip, size_t *flip_offset)
{
    uint32_t type;

    *flip = 0;

    if (test_random() % NOISE_RATE != 0) {
        *reinject = 0;
        *should_drop = 0;
        return;
    }

    type = test_random() % NUM_NOISE_TYPES;

    /*
     * Of noisy datagrams, 25% drop, 25% duplicate, 25% delay, 25% flip bits
     * A duplicated datagram keeps the current datagram and reinjects a new
     * identical one after up to MAX_DGRAM_DELAY datagrams have been sent.
     * A delayed datagram is implemented as both a reinject and a drop, i.e. an
     * identical datagram is reinjected after the given number of datagrams have
     * been sent and the current datagram is dropped.
     */
    *should_drop = (type == NOISE_TYPE_DROP || type == NOISE_TYPE_DELAY);

    /*
     * Where a duplicate occurs we reinject the copy of the datagram up to
     * MAX_DGRAM_DELAY datagrams later
     */
    *reinject = (type == NOISE_TYPE_DUPLICATE || type == NOISE_TYPE_DELAY)
                ? (uint64_t)((test_random() % MAX_DGRAM_REINJECT) + 1)
                : 0;

    /*
     * No point in reinjecting after 1 datagram if the current datagram is also
     * dropped (i.e. this is a delay not a duplicate), so we reinject after an
     * extra datagram in that case
     */
    *reinject += type == NOISE_TYPE_DELAY;

    /* flip some bits in the header */
    if (type == NOISE_TYPE_BITFLIPS) {
        /* we flip at most 8 bits of the 16 bit value at once */
        *flip = (test_random() % 255 + 1) << (test_random() % 8);
        /*
         * 25/50 bytes of guesstimated header size (it depends on CID length)
         * It does not matter much if it is overestimated.
         */
        *flip_offset = test_random() % (25 * (1 + long_header));
    }
}

static void flip_bits(unsigned char *msg, size_t msg_len, uint16_t flip,
                      size_t flip_offset)
{
    if (flip == 0)
        return;

    /* None of these border conditions should happen but check them anyway */
    if (msg_len < 2)
        return;
    if (msg_len < flip_offset + 2)
        flip_offset = msg_len - 2;

#ifdef OSSL_NOISY_DGRAM_DEBUG
    printf("**Flipping bits in a datagram at offset %u\n",
            (unsigned int)flip_offset);
    BIO_dump_fp(stdout, msg, msg_len);
    printf("\n");
#endif

    msg[flip_offset] ^= flip >> 8;
    msg[flip_offset + 1] ^= flip & 0xff;
}

static int noisy_dgram_recvmmsg(BIO *bio, BIO_MSG *msg, size_t stride,
                                size_t num_msg, uint64_t flags,
                                size_t *msgs_processed)
{
    BIO *next = BIO_next(bio);
    size_t i, j, data_len = 0, msg_cnt = 0;
    BIO_MSG *thismsg;
    struct noisy_dgram_st *data;

    if (!TEST_ptr(next))
        return 0;

    data = BIO_get_data(bio);
    if (!TEST_ptr(data))
        return 0;

    /*
     * For simplicity we assume that all elements in the msg array have the
     * same data_len. They are not required to by the API, but it would be quite
     * strange for that not to be the case - and our code that calls
     * BIO_recvmmsg does do this (which is all that is important for this test
     * code). We test the invariant here.
     */
    for (i = 0; i < num_msg; i++) {
        if (i == 0) {
            data_len = msg[i].data_len;
            if (!TEST_size_t_le(data_len, MSG_DATA_LEN_MAX))
                return 0;
        } else if (!TEST_size_t_eq(msg[i].data_len, data_len)) {
            return 0;
        }
    }

    if (!BIO_recvmmsg(next, msg, stride, num_msg, flags, msgs_processed))
        return 0;

#ifdef OSSL_NOISY_DGRAM_DEBUG
    printf("Pre-filter datagram list:\n");
    for (i = 0; i < *msgs_processed; i++) {
        printf("Pre-filter Datagram:\n");
        BIO_dump_fp(stdout, msg[i].data, msg[i].data_len);
        printf("\n");
    }
    printf("End of pre-filter datagram list\nApplying noise filters:\n");
#endif

    msg_cnt = *msgs_processed;

    /* Introduce noise */
    for (i = 0, thismsg = msg;
         i < msg_cnt;
         i++, thismsg++, data->this_dgram++) {
        uint64_t reinject;
        int should_drop;
        uint16_t flip;
        size_t flip_offset;

        /* If we have a message to reinject then insert it now */
        if (data->reinject_dgram > 0
                && data->reinject_dgram == data->this_dgram) {
            if (msg_cnt < num_msg) {
                /* Make space for the injected message */
                for (j = msg_cnt; j > i; j--) {
                    if (!bio_msg_copy(&msg[j], &msg[j - 1]))
                        return 0;
                }
                if (!bio_msg_copy(thismsg, &data->msg))
                    return 0;
                msg_cnt++;
                data->reinject_dgram = 0;
#ifdef OSSL_NOISY_DGRAM_DEBUG
                printf("**Injecting a datagram\n");
                BIO_dump_fp(stdout, thismsg->data, thismsg->data_len);
                printf("\n");
#endif
                continue;
            } /* else we have no space for the injection, so just drop it */
            data->reinject_dgram = 0;
        }

        get_noise(/* long header */ (((uint8_t *)thismsg->data)[0] & 0x80) != 0,
                  &reinject, &should_drop, &flip, &flip_offset);
        if (data->backoff) {
            /*
             * We might be asked to back off on introducing too much noise if
             * there is a danger that the connection will fail. In that case
             * we always ensure that the next datagram does not get dropped so
             * that the connection always survives. After that we can resume
             * with normal noise
             */
#ifdef OSSL_NOISY_DGRAM_DEBUG
            printf("**Back off applied\n");
#endif
            should_drop = 0;
            flip = 0;
            data->backoff = 0;
        }

        flip_bits(thismsg->data, thismsg->data_len, flip, flip_offset);

        /*
         * We ignore reinjection if a message is already waiting to be
         * reinjected
         */
        if (reinject > 0 && data->reinject_dgram == 0) {
            /*
             * Both duplicated and delayed datagrams get reintroduced after the
             * delay period. Datagrams that are delayed only (not duplicated)
             * will also have the current copy of the datagram dropped (i.e
             * should_drop below will be true).
             */
            if (!bio_msg_copy(&data->msg, thismsg))
                return 0;

            data->reinject_dgram = data->this_dgram + reinject;

#ifdef OSSL_NOISY_DGRAM_DEBUG
            printf("**Scheduling a reinject after %u messages%s\n",
                   (unsigned int)reinject, should_drop ? "" : "(duplicating)");
            BIO_dump_fp(stdout, thismsg->data, thismsg->data_len);
            printf("\n");
#endif
        }

        if (should_drop) {
#ifdef OSSL_NOISY_DGRAM_DEBUG
            printf("**Dropping a datagram\n");
            BIO_dump_fp(stdout, thismsg->data, thismsg->data_len);
            printf("\n");
#endif
            for (j = i + 1; j < msg_cnt; j++) {
                if (!bio_msg_copy(&msg[j - 1], &msg[j]))
                    return 0;
            }
            msg_cnt--;
        }
    }

#ifdef OSSL_NOISY_DGRAM_DEBUG
    printf("End of noise filters\nPost-filter datagram list:\n");
    for (i = 0; i < msg_cnt; i++) {
        printf("Post-filter Datagram:\n");
        BIO_dump_fp(stdout, msg[i].data, msg[i].data_len);
        printf("\n");
    }
    printf("End of post-filter datagram list\n");
#endif

    *msgs_processed = msg_cnt;

    if (msg_cnt == 0) {
        ERR_raise(ERR_LIB_BIO, BIO_R_NON_FATAL);
        return 0;
    }

    return 1;
}

static void data_free(struct noisy_dgram_st *data)
{
    if (data == NULL)
        return;

    OPENSSL_free(data->msg.data);
    BIO_ADDR_free(data->msg.peer);
    BIO_ADDR_free(data->msg.local);
    OPENSSL_free(data);
}

static int noisy_dgram_new(BIO *bio)
{
    struct noisy_dgram_st *data = OPENSSL_zalloc(sizeof(*data));

    if (!TEST_ptr(data))
        return 0;

    data->msg.data = OPENSSL_malloc(MSG_DATA_LEN_MAX);
    data->msg.peer = BIO_ADDR_new();
    data->msg.local = BIO_ADDR_new();
    if (data->msg.data == NULL
            || data->msg.peer == NULL
            || data->msg.local == NULL) {
        data_free(data);
        return 0;
    }

    BIO_set_data(bio, data);
    BIO_set_init(bio, 1);

    return 1;
}

static int noisy_dgram_free(BIO *bio)
{
    data_free(BIO_get_data(bio));
    BIO_set_data(bio, NULL);
    BIO_set_init(bio, 0);

    return 1;
}

/* Choose a sufficiently large type likely to be unused for this custom BIO */
#define BIO_TYPE_NOISY_DGRAM_FILTER  (0x80 | BIO_TYPE_FILTER)

static BIO_METHOD *method_noisy_dgram = NULL;

/* Note: Not thread safe! */
const BIO_METHOD *bio_f_noisy_dgram_filter(void)
{
    if (method_noisy_dgram == NULL) {
        method_noisy_dgram = BIO_meth_new(BIO_TYPE_NOISY_DGRAM_FILTER,
                                          "Nosiy datagram filter");
        if (method_noisy_dgram == NULL
            || !BIO_meth_set_ctrl(method_noisy_dgram, noisy_dgram_ctrl)
            || !BIO_meth_set_sendmmsg(method_noisy_dgram, noisy_dgram_sendmmsg)
            || !BIO_meth_set_recvmmsg(method_noisy_dgram, noisy_dgram_recvmmsg)
            || !BIO_meth_set_create(method_noisy_dgram, noisy_dgram_new)
            || !BIO_meth_set_destroy(method_noisy_dgram, noisy_dgram_free))
            return NULL;
    }
    return method_noisy_dgram;
}

void bio_f_noisy_dgram_filter_free(void)
{
    BIO_meth_free(method_noisy_dgram);
}
