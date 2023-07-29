/*-
 * Copyright 2021-2022 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/* Private RSA key used for decryption */
static const unsigned char priv_key_der[] = {
    0x30, 0x82, 0x04, 0xa4, 0x02, 0x01, 0x00, 0x02, 0x82, 0x01, 0x01, 0x00,
    0xc2, 0x44, 0xbc, 0xcf, 0x5b, 0xca, 0xcd, 0x80, 0x77, 0xae, 0xf9, 0x7a,
    0x34, 0xbb, 0x37, 0x6f, 0x5c, 0x76, 0x4c, 0xe4, 0xbb, 0x0c, 0x1d, 0xe7,
    0xfe, 0x0f, 0xda, 0xcf, 0x8c, 0x56, 0x65, 0x72, 0x6e, 0x2c, 0xf9, 0xfd,
    0x87, 0x43, 0xeb, 0x4c, 0x26, 0xb1, 0xd3, 0xf0, 0x87, 0xb1, 0x18, 0x68,
    0x14, 0x7d, 0x3c, 0x2a, 0xfa, 0xc2, 0x5d, 0x70, 0x19, 0x11, 0x00, 0x2e,
    0xb3, 0x9c, 0x8e, 0x38, 0x08, 0xbe, 0xe3, 0xeb, 0x7d, 0x6e, 0xc7, 0x19,
    0xc6, 0x7f, 0x59, 0x48, 0x84, 0x1b, 0xe3, 0x27, 0x30, 0x46, 0x30, 0xd3,
    0xfc, 0xfc, 0xb3, 0x35, 0x75, 0xc4, 0x31, 0x1a, 0xc0, 0xc2, 0x4c, 0x0b,
    0xc7, 0x01, 0x95, 0xb2, 0xdc, 0x17, 0x77, 0x9b, 0x09, 0x15, 0x04, 0xbc,
    0xdb, 0x57, 0x0b, 0x26, 0xda, 0x59, 0x54, 0x0d, 0x6e, 0xb7, 0x89, 0xbc,
    0x53, 0x9d, 0x5f, 0x8c, 0xad, 0x86, 0x97, 0xd2, 0x48, 0x4f, 0x5c, 0x94,
    0xdd, 0x30, 0x2f, 0xcf, 0xfc, 0xde, 0x20, 0x31, 0x25, 0x9d, 0x29, 0x25,
    0x78, 0xb7, 0xd2, 0x5b, 0x5d, 0x99, 0x5b, 0x08, 0x12, 0x81, 0x79, 0x89,
    0xa0, 0xcf, 0x8f, 0x40, 0xb1, 0x77, 0x72, 0x3b, 0x13, 0xfc, 0x55, 0x43,
    0x70, 0x29, 0xd5, 0x41, 0xed, 0x31, 0x4b, 0x2d, 0x6c, 0x7d, 0xcf, 0x99,
    0x5f, 0xd1, 0x72, 0x9f, 0x8b, 0x32, 0x96, 0xde, 0x5d, 0x8b, 0x19, 0x77,
    0x75, 0xff, 0x09, 0xbf, 0x26, 0xe9, 0xd7, 0x3d, 0xc7, 0x1a, 0x81, 0xcf,
    0x05, 0x1b, 0x89, 0xbf, 0x45, 0x32, 0xbf, 0x5e, 0xc9, 0xe3, 0x5c, 0x33,
    0x4a, 0x72, 0x47, 0xf4, 0x24, 0xae, 0x9b, 0x38, 0x24, 0x76, 0x9a, 0xa2,
    0x9a, 0x50, 0x50, 0x49, 0xf5, 0x26, 0xb9, 0x55, 0xa6, 0x47, 0xc9, 0x14,
    0xa2, 0xca, 0xd4, 0xa8, 0x8a, 0x9f, 0xe9, 0x5a, 0x5a, 0x12, 0xaa, 0x30,
    0xd5, 0x78, 0x8b, 0x39, 0x02, 0x03, 0x01, 0x00, 0x01, 0x02, 0x82, 0x01,
    0x00, 0x22, 0x5d, 0xb9, 0x8e, 0xef, 0x1c, 0x91, 0xbd, 0x03, 0xaf, 0x1a,
    0xe8, 0x00, 0xf3, 0x0b, 0x8b, 0xf2, 0x2d, 0xe5, 0x4d, 0x63, 0x3f, 0x71,
    0xfc, 0xeb, 0xc7, 0x4f, 0x3c, 0x7f, 0x05, 0x7b, 0x9d, 0xc2, 0x1a, 0xc7,
    0xc0, 0x8f, 0x50, 0xb7, 0x0b, 0xba, 0x1e, 0xa4, 0x30, 0xfd, 0x38, 0x19,
    0x6a, 0xb4, 0x11, 0x31, 0x77, 0x22, 0xf4, 0x06, 0x46, 0x81, 0xd0, 0xad,
    0x99, 0x15, 0x62, 0x01, 0x10, 0xad, 0x8f, 0x63, 0x4f, 0x71, 0xd9, 0x8a,
    0x74, 0x27, 0x56, 0xb8, 0xeb, 0x28, 0x9f, 0xac, 0x4f, 0xee, 0xec, 0xc3,
    0xcf, 0x84, 0x86, 0x09, 0x87, 0xd0, 0x04, 0xfc, 0x70, 0xd0, 0x9f, 0xae,
    0x87, 0x38, 0xd5, 0xb1, 0x6f, 0x3a, 0x1b, 0x16, 0xa8, 0x00, 0xf3, 0xcc,
    0x6a, 0x42, 0x5d, 0x04, 0x16, 0x83, 0xf2, 0xe0, 0x79, 0x1d, 0xd8, 0x6f,
    0x0f, 0xb7, 0x34, 0xf4, 0x45, 0xb5, 0x1e, 0xc5, 0xb5, 0x78, 0xa7, 0xd3,
    0xa3, 0x23, 0x35, 0xbc, 0x7b, 0x01, 0x59, 0x7d, 0xee, 0xb9, 0x4f, 0xda,
    0x28, 0xad, 0x5d, 0x25, 0xab, 0x66, 0x6a, 0xb0, 0x61, 0xf6, 0x12, 0xa7,
    0xee, 0xd1, 0xe7, 0xb1, 0x8b, 0x91, 0x29, 0xba, 0xb5, 0xf8, 0x78, 0xc8,
    0x6b, 0x76, 0x67, 0x32, 0xe8, 0xf3, 0x4e, 0x59, 0xba, 0xc1, 0x44, 0xc0,
    0xec, 0x8d, 0x7c, 0x63, 0xb2, 0x6e, 0x0c, 0xb9, 0x33, 0x42, 0x0c, 0x8d,
    0xae, 0x4e, 0x54, 0xc8, 0x8a, 0xef, 0xf9, 0x47, 0xc8, 0x99, 0x84, 0xc8,
    0x46, 0xf6, 0xa6, 0x53, 0x59, 0xf8, 0x60, 0xe3, 0xd7, 0x1d, 0x10, 0x95,
    0xf5, 0x6d, 0xf4, 0xa3, 0x18, 0x40, 0xd7, 0x14, 0x04, 0xac, 0x8c, 0x69,
    0xd6, 0x14, 0xdc, 0xd8, 0xcc, 0xbc, 0x1c, 0xac, 0xd7, 0x21, 0x2b, 0x7e,
    0x29, 0x88, 0x06, 0xa0, 0xf4, 0x06, 0x08, 0x14, 0x04, 0x4d, 0x32, 0x33,
    0x84, 0x9c, 0x20, 0x8e, 0xcf, 0x02, 0x81, 0x81, 0x00, 0xf3, 0xf9, 0xbd,
    0xd5, 0x43, 0x6f, 0x27, 0x4a, 0x92, 0xd6, 0x18, 0x3d, 0x4b, 0xf1, 0x77,
    0x7c, 0xaf, 0xce, 0x01, 0x17, 0x98, 0xcb, 0xbe, 0x06, 0x86, 0x3a, 0x13,
    0x72, 0x4b, 0x7c, 0x81, 0x51, 0x24, 0x5d, 0xc3, 0xe9, 0xa2, 0x63, 0x1e,
    0x4a, 0xeb, 0x66, 0xae, 0x01, 0x5e, 0xa4, 0xa4, 0x74, 0x9e, 0xee, 0x32,
    0xe5, 0x59, 0x1b, 0x37, 0xef, 0x7d, 0xb3, 0x42, 0x8c, 0x93, 0x8b, 0xd3,
    0x1e, 0x83, 0x43, 0xb5, 0x88, 0x3e, 0x24, 0xeb, 0xdc, 0x92, 0x2d, 0xcc,
    0x9a, 0x9d, 0xf1, 0x7d, 0x16, 0x71, 0xcb, 0x25, 0x47, 0x36, 0xb0, 0xc4,
    0x6b, 0xc8, 0x53, 0x4a, 0x25, 0x80, 0x47, 0x77, 0xdb, 0x97, 0x13, 0x15,
    0x0f, 0x4a, 0xfa, 0x0c, 0x6c, 0x44, 0x13, 0x2f, 0xbc, 0x9a, 0x6b, 0x13,
    0x57, 0xfc, 0x42, 0xb9, 0xe9, 0xd3, 0x2e, 0xd2, 0x11, 0xf4, 0xc5, 0x84,
    0x55, 0xd2, 0xdf, 0x1d, 0xa7, 0x02, 0x81, 0x81, 0x00, 0xcb, 0xd7, 0xd6,
    0x9d, 0x71, 0xb3, 0x86, 0xbe, 0x68, 0xed, 0x67, 0xe1, 0x51, 0x92, 0x17,
    0x60, 0x58, 0xb3, 0x2a, 0x56, 0xfd, 0x18, 0xfb, 0x39, 0x4b, 0x14, 0xc6,
    0xf6, 0x67, 0x0e, 0x31, 0xe3, 0xb3, 0x2f, 0x1f, 0xec, 0x16, 0x1c, 0x23,
    0x2b, 0x60, 0x36, 0xd1, 0xcb, 0x4a, 0x03, 0x6a, 0x3a, 0x4c, 0x8c, 0xf2,
    0x73, 0x08, 0x23, 0x29, 0xda, 0xcb, 0xf7, 0xb6, 0x18, 0x97, 0xc6, 0xfe,
    0xd4, 0x40, 0x06, 0x87, 0x9d, 0x6e, 0xbb, 0x5d, 0x14, 0x44, 0xc8, 0x19,
    0xfa, 0x7f, 0x0c, 0xc5, 0x02, 0x92, 0x00, 0xbb, 0x2e, 0x4f, 0x50, 0xb0,
    0x71, 0x9f, 0xf3, 0x94, 0x12, 0xb8, 0x6c, 0x5f, 0xe1, 0x83, 0x7b, 0xbc,
    0x8c, 0x0a, 0x6f, 0x09, 0x6a, 0x35, 0x4f, 0xf9, 0xa4, 0x92, 0x93, 0xe3,
    0xad, 0x36, 0x25, 0x28, 0x90, 0x85, 0xd2, 0x9f, 0x86, 0xfd, 0xd9, 0xa8,
    0x61, 0xe9, 0xb2, 0xec, 0x1f, 0x02, 0x81, 0x81, 0x00, 0xdd, 0x1c, 0x52,
    0xda, 0x2b, 0xc2, 0x5a, 0x26, 0xb0, 0xcb, 0x0d, 0xae, 0xc7, 0xdb, 0xf0,
    0x41, 0x75, 0x87, 0x4a, 0xe0, 0x1a, 0xdf, 0x53, 0xb9, 0xcf, 0xfe, 0x64,
    0x4f, 0x6a, 0x70, 0x4d, 0x36, 0xbf, 0xb1, 0xa6, 0xf3, 0x5f, 0xf3, 0x5a,
    0xa9, 0xe5, 0x8b, 0xea, 0x59, 0x5d, 0x6f, 0xf3, 0x87, 0xa9, 0xde, 0x11,
    0x0c, 0x60, 0x64, 0x55, 0x9e, 0x5c, 0x1a, 0x91, 0x4e, 0x9c, 0x0d, 0xd5,
    0xe9, 0x4a, 0x67, 0x9b, 0xe6, 0xfd, 0x03, 0x33, 0x2b, 0x74, 0xe3, 0xc3,
    0x11, 0xc1, 0xe0, 0xf1, 0x4f, 0xdd, 0x13, 0x92, 0x16, 0x67, 0x4f, 0x6e,
    0xc4, 0x8c, 0x0a, 0x48, 0x21, 0x92, 0x8f, 0xb2, 0xe5, 0xb5, 0x96, 0x5a,
    0xb8, 0xc0, 0x67, 0xbb, 0xc8, 0x87, 0x2d, 0xa8, 0x4e, 0xd2, 0xd8, 0x05,
    0xf0, 0xf0, 0xb3, 0x7c, 0x90, 0x98, 0x8f, 0x4f, 0x5d, 0x6c, 0xab, 0x71,
    0x92, 0xe2, 0x88, 0xc8, 0xf3, 0x02, 0x81, 0x81, 0x00, 0x99, 0x27, 0x5a,
    0x00, 0x81, 0x65, 0x39, 0x5f, 0xe6, 0xc6, 0x38, 0xbe, 0x79, 0xe3, 0x21,
    0xdd, 0x29, 0xc7, 0xb3, 0x90, 0x18, 0x29, 0xa4, 0xd7, 0xaf, 0x29, 0xb5,
    0x33, 0x7c, 0xca, 0x95, 0x81, 0x57, 0x27, 0x98, 0xfc, 0x70, 0xc0, 0x43,
    0x4c, 0x5b, 0xc5, 0xd4, 0x6a, 0xc0, 0xf9, 0x3f, 0xde, 0xfd, 0x95, 0x08,
    0xb4, 0x94, 0xf0, 0x96, 0x89, 0xe5, 0xa6, 0x00, 0x13, 0x0a, 0x36, 0x61,
    0x50, 0x67, 0xaa, 0x80, 0x4a, 0x30, 0xe0, 0x65, 0x56, 0xcd, 0x36, 0xeb,
    0x0d, 0xe2, 0x57, 0x5d, 0xce, 0x48, 0x94, 0x74, 0x0e, 0x9f, 0x59, 0x28,
    0xb8, 0xb6, 0x4c, 0xf4, 0x7b, 0xfc, 0x44, 0xb0, 0xe5, 0x67, 0x3c, 0x98,
    0xb5, 0x3f, 0x41, 0x9d, 0xf9, 0x46, 0x85, 0x08, 0x34, 0x36, 0x4d, 0x17,
    0x4b, 0x14, 0xdb, 0x66, 0x56, 0xef, 0xb5, 0x08, 0x57, 0x0c, 0x73, 0x74,
    0xa7, 0xdc, 0x46, 0xaa, 0x51, 0x02, 0x81, 0x80, 0x1e, 0x50, 0x4c, 0xde,
    0x9c, 0x60, 0x6d, 0xd7, 0x31, 0xf6, 0xd8, 0x4f, 0xc2, 0x25, 0x7d, 0x83,
    0xb3, 0xe7, 0xed, 0x92, 0xe7, 0x28, 0x1e, 0xb3, 0x9b, 0xcb, 0xf2, 0x86,
    0xa4, 0x49, 0x45, 0x5e, 0xba, 0x1d, 0xdb, 0x21, 0x5d, 0xdf, 0xeb, 0x3c,
    0x5e, 0x01, 0xc6, 0x68, 0x25, 0x28, 0xe6, 0x1a, 0xbf, 0xc1, 0xa1, 0xc5,
    0x92, 0x0b, 0x08, 0x43, 0x0e, 0x5a, 0xa3, 0x85, 0x8a, 0x65, 0xb4, 0x54,
    0xa1, 0x4c, 0x20, 0xa2, 0x5a, 0x08, 0xf6, 0x90, 0x0d, 0x9a, 0xd7, 0x20,
    0xf1, 0x10, 0x66, 0x28, 0x4c, 0x22, 0x56, 0xa6, 0xb9, 0xff, 0xd0, 0x6a,
    0x62, 0x8c, 0x9f, 0xf8, 0x7c, 0xf4, 0xad, 0xd7, 0xe8, 0xf9, 0x87, 0x43,
    0xbf, 0x73, 0x5b, 0x04, 0xc7, 0xd0, 0x77, 0xcc, 0xe3, 0xbe, 0xda, 0xc2,
    0x07, 0xed, 0x8d, 0x2a, 0x15, 0x77, 0x1d, 0x53, 0x47, 0xe0, 0xa2, 0x11,
    0x41, 0x0d, 0xe2, 0xe7,
};

/* The matching public key used for encryption*/
static const unsigned char pub_key_der[] = {
    0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
    0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00,
    0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xc2, 0x44, 0xbc,
    0xcf, 0x5b, 0xca, 0xcd, 0x80, 0x77, 0xae, 0xf9, 0x7a, 0x34, 0xbb, 0x37,
    0x6f, 0x5c, 0x76, 0x4c, 0xe4, 0xbb, 0x0c, 0x1d, 0xe7, 0xfe, 0x0f, 0xda,
    0xcf, 0x8c, 0x56, 0x65, 0x72, 0x6e, 0x2c, 0xf9, 0xfd, 0x87, 0x43, 0xeb,
    0x4c, 0x26, 0xb1, 0xd3, 0xf0, 0x87, 0xb1, 0x18, 0x68, 0x14, 0x7d, 0x3c,
    0x2a, 0xfa, 0xc2, 0x5d, 0x70, 0x19, 0x11, 0x00, 0x2e, 0xb3, 0x9c, 0x8e,
    0x38, 0x08, 0xbe, 0xe3, 0xeb, 0x7d, 0x6e, 0xc7, 0x19, 0xc6, 0x7f, 0x59,
    0x48, 0x84, 0x1b, 0xe3, 0x27, 0x30, 0x46, 0x30, 0xd3, 0xfc, 0xfc, 0xb3,
    0x35, 0x75, 0xc4, 0x31, 0x1a, 0xc0, 0xc2, 0x4c, 0x0b, 0xc7, 0x01, 0x95,
    0xb2, 0xdc, 0x17, 0x77, 0x9b, 0x09, 0x15, 0x04, 0xbc, 0xdb, 0x57, 0x0b,
    0x26, 0xda, 0x59, 0x54, 0x0d, 0x6e, 0xb7, 0x89, 0xbc, 0x53, 0x9d, 0x5f,
    0x8c, 0xad, 0x86, 0x97, 0xd2, 0x48, 0x4f, 0x5c, 0x94, 0xdd, 0x30, 0x2f,
    0xcf, 0xfc, 0xde, 0x20, 0x31, 0x25, 0x9d, 0x29, 0x25, 0x78, 0xb7, 0xd2,
    0x5b, 0x5d, 0x99, 0x5b, 0x08, 0x12, 0x81, 0x79, 0x89, 0xa0, 0xcf, 0x8f,
    0x40, 0xb1, 0x77, 0x72, 0x3b, 0x13, 0xfc, 0x55, 0x43, 0x70, 0x29, 0xd5,
    0x41, 0xed, 0x31, 0x4b, 0x2d, 0x6c, 0x7d, 0xcf, 0x99, 0x5f, 0xd1, 0x72,
    0x9f, 0x8b, 0x32, 0x96, 0xde, 0x5d, 0x8b, 0x19, 0x77, 0x75, 0xff, 0x09,
    0xbf, 0x26, 0xe9, 0xd7, 0x3d, 0xc7, 0x1a, 0x81, 0xcf, 0x05, 0x1b, 0x89,
    0xbf, 0x45, 0x32, 0xbf, 0x5e, 0xc9, 0xe3, 0x5c, 0x33, 0x4a, 0x72, 0x47,
    0xf4, 0x24, 0xae, 0x9b, 0x38, 0x24, 0x76, 0x9a, 0xa2, 0x9a, 0x50, 0x50,
    0x49, 0xf5, 0x26, 0xb9, 0x55, 0xa6, 0x47, 0xc9, 0x14, 0xa2, 0xca, 0xd4,
    0xa8, 0x8a, 0x9f, 0xe9, 0x5a, 0x5a, 0x12, 0xaa, 0x30, 0xd5, 0x78, 0x8b,
    0x39, 0x02, 0x03, 0x01, 0x00, 0x01,
};
