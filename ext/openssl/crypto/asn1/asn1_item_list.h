/*
 * Copyright 2000-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

static ASN1_ITEM_EXP *asn1_item_list[] = {

    ASN1_ITEM_ref(ACCESS_DESCRIPTION),
#ifndef OPENSSL_NO_RFC3779
    ASN1_ITEM_ref(ASIdOrRange),
    ASN1_ITEM_ref(ASIdentifierChoice),
    ASN1_ITEM_ref(ASIdentifiers),
#endif
    ASN1_ITEM_ref(ASN1_ANY),
    ASN1_ITEM_ref(ASN1_BIT_STRING),
    ASN1_ITEM_ref(ASN1_BMPSTRING),
    ASN1_ITEM_ref(ASN1_BOOLEAN),
    ASN1_ITEM_ref(ASN1_ENUMERATED),
    ASN1_ITEM_ref(ASN1_FBOOLEAN),
    ASN1_ITEM_ref(ASN1_GENERALIZEDTIME),
    ASN1_ITEM_ref(ASN1_GENERALSTRING),
    ASN1_ITEM_ref(ASN1_IA5STRING),
    ASN1_ITEM_ref(ASN1_INTEGER),
    ASN1_ITEM_ref(ASN1_NULL),
    ASN1_ITEM_ref(ASN1_OBJECT),
    ASN1_ITEM_ref(ASN1_OCTET_STRING_NDEF),
    ASN1_ITEM_ref(ASN1_OCTET_STRING),
    ASN1_ITEM_ref(ASN1_PRINTABLESTRING),
    ASN1_ITEM_ref(ASN1_PRINTABLE),
    ASN1_ITEM_ref(ASN1_SEQUENCE_ANY),
    ASN1_ITEM_ref(ASN1_SEQUENCE),
    ASN1_ITEM_ref(ASN1_SET_ANY),
    ASN1_ITEM_ref(ASN1_T61STRING),
    ASN1_ITEM_ref(ASN1_TBOOLEAN),
    ASN1_ITEM_ref(ASN1_TIME),
    ASN1_ITEM_ref(ASN1_UNIVERSALSTRING),
    ASN1_ITEM_ref(ASN1_UTCTIME),
    ASN1_ITEM_ref(ASN1_UTF8STRING),
    ASN1_ITEM_ref(ASN1_VISIBLESTRING),
#ifndef OPENSSL_NO_RFC3779
    ASN1_ITEM_ref(ASRange),
#endif
    ASN1_ITEM_ref(AUTHORITY_INFO_ACCESS),
    ASN1_ITEM_ref(AUTHORITY_KEYID),
    ASN1_ITEM_ref(BASIC_CONSTRAINTS),
    ASN1_ITEM_ref(BIGNUM),
    ASN1_ITEM_ref(CBIGNUM),
    ASN1_ITEM_ref(CERTIFICATEPOLICIES),
#ifndef OPENSSL_NO_CMS
    ASN1_ITEM_ref(CMS_ContentInfo),
    ASN1_ITEM_ref(CMS_EnvelopedData),
    ASN1_ITEM_ref(CMS_ReceiptRequest),
#endif
    ASN1_ITEM_ref(CRL_DIST_POINTS),
#ifndef OPENSSL_NO_DH
    ASN1_ITEM_ref(DHparams),
#endif
    ASN1_ITEM_ref(DIRECTORYSTRING),
    ASN1_ITEM_ref(DISPLAYTEXT),
    ASN1_ITEM_ref(DIST_POINT_NAME),
    ASN1_ITEM_ref(DIST_POINT),
#ifndef OPENSSL_NO_EC
# ifndef OPENSSL_NO_DEPRECATED_3_0
    ASN1_ITEM_ref(ECPARAMETERS),
    ASN1_ITEM_ref(ECPKPARAMETERS),
# endif
#endif
    ASN1_ITEM_ref(EDIPARTYNAME),
    ASN1_ITEM_ref(EXTENDED_KEY_USAGE),
    ASN1_ITEM_ref(GENERAL_NAMES),
    ASN1_ITEM_ref(GENERAL_NAME),
    ASN1_ITEM_ref(GENERAL_SUBTREE),
#ifndef OPENSSL_NO_RFC3779
    ASN1_ITEM_ref(IPAddressChoice),
    ASN1_ITEM_ref(IPAddressFamily),
    ASN1_ITEM_ref(IPAddressOrRange),
    ASN1_ITEM_ref(IPAddressRange),
#endif
    ASN1_ITEM_ref(ISSUING_DIST_POINT),
#ifndef OPENSSL_NO_DEPRECATED_3_0
    ASN1_ITEM_ref(LONG),
#endif
    ASN1_ITEM_ref(NAME_CONSTRAINTS),
    ASN1_ITEM_ref(NETSCAPE_CERT_SEQUENCE),
    ASN1_ITEM_ref(NETSCAPE_SPKAC),
    ASN1_ITEM_ref(NETSCAPE_SPKI),
    ASN1_ITEM_ref(NOTICEREF),
#ifndef OPENSSL_NO_OCSP
    ASN1_ITEM_ref(OCSP_BASICRESP),
    ASN1_ITEM_ref(OCSP_CERTID),
    ASN1_ITEM_ref(OCSP_CERTSTATUS),
    ASN1_ITEM_ref(OCSP_CRLID),
    ASN1_ITEM_ref(OCSP_ONEREQ),
    ASN1_ITEM_ref(OCSP_REQINFO),
    ASN1_ITEM_ref(OCSP_REQUEST),
    ASN1_ITEM_ref(OCSP_RESPBYTES),
    ASN1_ITEM_ref(OCSP_RESPDATA),
    ASN1_ITEM_ref(OCSP_RESPID),
    ASN1_ITEM_ref(OCSP_RESPONSE),
    ASN1_ITEM_ref(OCSP_REVOKEDINFO),
    ASN1_ITEM_ref(OCSP_SERVICELOC),
    ASN1_ITEM_ref(OCSP_SIGNATURE),
    ASN1_ITEM_ref(OCSP_SINGLERESP),
#endif
    ASN1_ITEM_ref(OTHERNAME),
    ASN1_ITEM_ref(PBE2PARAM),
    ASN1_ITEM_ref(PBEPARAM),
    ASN1_ITEM_ref(PBKDF2PARAM),
    ASN1_ITEM_ref(PKCS12_AUTHSAFES),
    ASN1_ITEM_ref(PKCS12_BAGS),
    ASN1_ITEM_ref(PKCS12_MAC_DATA),
    ASN1_ITEM_ref(PKCS12_SAFEBAGS),
    ASN1_ITEM_ref(PKCS12_SAFEBAG),
    ASN1_ITEM_ref(PKCS12),
    ASN1_ITEM_ref(PKCS7_ATTR_SIGN),
    ASN1_ITEM_ref(PKCS7_ATTR_VERIFY),
    ASN1_ITEM_ref(PKCS7_DIGEST),
    ASN1_ITEM_ref(PKCS7_ENCRYPT),
    ASN1_ITEM_ref(PKCS7_ENC_CONTENT),
    ASN1_ITEM_ref(PKCS7_ENVELOPE),
    ASN1_ITEM_ref(PKCS7_ISSUER_AND_SERIAL),
    ASN1_ITEM_ref(PKCS7_RECIP_INFO),
    ASN1_ITEM_ref(PKCS7_SIGNED),
    ASN1_ITEM_ref(PKCS7_SIGNER_INFO),
    ASN1_ITEM_ref(PKCS7_SIGN_ENVELOPE),
    ASN1_ITEM_ref(PKCS7),
    ASN1_ITEM_ref(PKCS8_PRIV_KEY_INFO),
    ASN1_ITEM_ref(PKEY_USAGE_PERIOD),
    ASN1_ITEM_ref(POLICYINFO),
    ASN1_ITEM_ref(POLICYQUALINFO),
    ASN1_ITEM_ref(POLICY_CONSTRAINTS),
    ASN1_ITEM_ref(POLICY_MAPPINGS),
    ASN1_ITEM_ref(POLICY_MAPPING),
    ASN1_ITEM_ref(PROXY_CERT_INFO_EXTENSION),
    ASN1_ITEM_ref(PROXY_POLICY),
#ifndef OPENSSL_NO_DEPRECATED_3_0
    ASN1_ITEM_ref(RSAPrivateKey),
    ASN1_ITEM_ref(RSAPublicKey),
    ASN1_ITEM_ref(RSA_OAEP_PARAMS),
    ASN1_ITEM_ref(RSA_PSS_PARAMS),
#endif
#ifndef OPENSSL_NO_SCRYPT
    ASN1_ITEM_ref(SCRYPT_PARAMS),
#endif
    ASN1_ITEM_ref(SXNETID),
    ASN1_ITEM_ref(SXNET),
    ASN1_ITEM_ref(ISSUER_SIGN_TOOL),
    ASN1_ITEM_ref(USERNOTICE),
    ASN1_ITEM_ref(X509_ALGORS),
    ASN1_ITEM_ref(X509_ALGOR),
    ASN1_ITEM_ref(X509_ATTRIBUTE),
    ASN1_ITEM_ref(X509_CERT_AUX),
    ASN1_ITEM_ref(X509_CINF),
    ASN1_ITEM_ref(X509_CRL_INFO),
    ASN1_ITEM_ref(X509_CRL),
    ASN1_ITEM_ref(X509_EXTENSIONS),
    ASN1_ITEM_ref(X509_EXTENSION),
    ASN1_ITEM_ref(X509_NAME_ENTRY),
    ASN1_ITEM_ref(X509_NAME),
    ASN1_ITEM_ref(X509_PUBKEY),
    ASN1_ITEM_ref(X509_REQ_INFO),
    ASN1_ITEM_ref(X509_REQ),
    ASN1_ITEM_ref(X509_REVOKED),
    ASN1_ITEM_ref(X509_SIG),
    ASN1_ITEM_ref(X509_VAL),
    ASN1_ITEM_ref(X509),
#ifndef OPENSSL_NO_DEPRECATED_3_0
    ASN1_ITEM_ref(ZLONG),
#endif
    ASN1_ITEM_ref(INT32),
    ASN1_ITEM_ref(UINT32),
    ASN1_ITEM_ref(ZINT32),
    ASN1_ITEM_ref(ZUINT32),
    ASN1_ITEM_ref(INT64),
    ASN1_ITEM_ref(UINT64),
    ASN1_ITEM_ref(ZINT64),
    ASN1_ITEM_ref(ZUINT64),
};
