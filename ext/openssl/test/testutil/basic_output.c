/*
 * Copyright 2017-2023 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include "../testutil.h"
#include "output.h"
#include "tu_local.h"

#include <openssl/crypto.h>
#include <openssl/bio.h>

/* These are available for any test program */
BIO *bio_out = NULL;
BIO *bio_err = NULL;

/* These are available for TAP output only (internally) */
static BIO *tap_out = NULL;
static BIO *tap_err = NULL;

#if defined(OPENSSL_THREADS)
static CRYPTO_RWLOCK *io_lock = NULL;
#endif

void test_open_streams(void)
{
    tap_out = BIO_new_fp(stdout, BIO_NOCLOSE | BIO_FP_TEXT);
    tap_err = BIO_new_fp(stderr, BIO_NOCLOSE | BIO_FP_TEXT);
#ifdef __VMS
    tap_out = BIO_push(BIO_new(BIO_f_linebuffer()), tap_out);
    tap_err = BIO_push(BIO_new(BIO_f_linebuffer()), tap_err);
#endif
    tap_out = BIO_push(BIO_new(BIO_f_prefix()), tap_out);
    tap_err = BIO_push(BIO_new(BIO_f_prefix()), tap_err);

    bio_out = BIO_push(BIO_new(BIO_f_prefix()), tap_out);
    bio_err = BIO_push(BIO_new(BIO_f_prefix()), tap_err);
    BIO_set_prefix(bio_out, "# ");
    BIO_set_prefix(bio_err, "# ");

#if defined(OPENSSL_THREADS)
    io_lock = CRYPTO_THREAD_lock_new();
#endif

    OPENSSL_assert(bio_out != NULL);
    OPENSSL_assert(bio_err != NULL);
#if defined(OPENSSL_THREADS)
    OPENSSL_assert(io_lock != NULL);
#endif
}

void test_adjust_streams_tap_level(int level)
{
    BIO_set_indent(tap_out, level);
    BIO_set_indent(tap_err, level);
}

void test_close_streams(void)
{
    /*
     * The rest of the chain is freed by the BIO_free_all() calls below, so
     * we only need to free the last one in the bio_out and bio_err chains.
     */
    BIO_free(bio_out);
    BIO_free(bio_err);

    BIO_free_all(tap_out);
    BIO_free_all(tap_err);

#if defined(OPENSSL_THREADS)
    CRYPTO_THREAD_lock_free(io_lock);
#endif
}

static ossl_inline void test_io_lock(void)
{
#if defined(OPENSSL_THREADS)
    OPENSSL_assert(CRYPTO_THREAD_write_lock(io_lock) > 0);
#endif
}

static ossl_inline void test_io_unlock(void)
{
#if defined(OPENSSL_THREADS)
    CRYPTO_THREAD_unlock(io_lock);
#endif
}

int test_vprintf_stdout(const char *fmt, va_list ap)
{
    int r;

    test_io_lock();
    r = BIO_vprintf(bio_out, fmt, ap);
    test_io_unlock();

    return r;
}

int test_vprintf_stderr(const char *fmt, va_list ap)
{
    int r;

    test_io_lock();
    r = BIO_vprintf(bio_err, fmt, ap);
    test_io_unlock();

    return r;
}

int test_flush_stdout(void)
{
    int r;

    test_io_lock();
    r = BIO_flush(bio_out);
    test_io_unlock();

    return r;
}

int test_flush_stderr(void)
{
    int r;

    test_io_lock();
    r = BIO_flush(bio_err);
    test_io_unlock();

    return r;
}

int test_vprintf_tapout(const char *fmt, va_list ap)
{
    int r;

    test_io_lock();
    r = BIO_vprintf(tap_out, fmt, ap);
    test_io_unlock();

    return r;
}

int test_vprintf_taperr(const char *fmt, va_list ap)
{
    int r;

    test_io_lock();
    r = BIO_vprintf(tap_err, fmt, ap);
    test_io_unlock();

    return r;
}

int test_flush_tapout(void)
{
    int r;

    test_io_lock();
    r = BIO_flush(tap_out);
    test_io_unlock();

    return r;
}

int test_flush_taperr(void)
{
    int r;

    test_io_lock();
    r = BIO_flush(tap_err);
    test_io_unlock();

    return r;
}
