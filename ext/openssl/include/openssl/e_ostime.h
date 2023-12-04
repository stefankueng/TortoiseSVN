/*
 * Copyright 2023 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef OPENSSL_E_OSTIME_H
# define OPENSSL_E_OSTIME_H
# pragma once

# include <openssl/macros.h>
# include <openssl/opensslconf.h>
# include <openssl/e_os2.h>

/*
 * This header guarantees that 'struct timeval' will be available. It includes
 * the minimum headers needed to facilitate this. This may still be a
 * substantial set of headers on some platforms (e.g. <winsock2.h> on Win32).
 */

# if defined(OPENSSL_SYS_WINDOWS)
#  include <winsock2.h>
# else
#  include <sys/time.h>
# endif

#endif
