/*
 * Copyright 1995-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/*****************************************************************************
 *                                                                           *
 * The following macros/functions are PRIVATE to the record layer. They      *
 * should NOT be used outside of the record layer.                           *
 *                                                                           *
 *****************************************************************************/

#define MAX_WARN_ALERT_COUNT    5

/* Functions/macros provided by the RECORD_LAYER component */

#define DTLS_RECORD_LAYER_get_r_epoch(rl)       ((rl)->d->r_epoch)
