/*
 * Copyright 2007-2023 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright Nokia 2007-2019
 * Copyright Siemens AG 2015-2019
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/asn1t.h>

#include "cmp_local.h"

/* explicit #includes not strictly needed since implied by the above: */
#include <openssl/cmp.h>
#include <openssl/crmf.h>

/* ASN.1 declarations from RFC4210 */
ASN1_SEQUENCE(OSSL_CMP_REVANNCONTENT) = {
    /* OSSL_CMP_PKISTATUS is effectively ASN1_INTEGER so it is used directly */
    ASN1_SIMPLE(OSSL_CMP_REVANNCONTENT, status, ASN1_INTEGER),
    ASN1_SIMPLE(OSSL_CMP_REVANNCONTENT, certId, OSSL_CRMF_CERTID),
    ASN1_SIMPLE(OSSL_CMP_REVANNCONTENT, willBeRevokedAt, ASN1_GENERALIZEDTIME),
    ASN1_SIMPLE(OSSL_CMP_REVANNCONTENT, badSinceDate, ASN1_GENERALIZEDTIME),
    ASN1_OPT(OSSL_CMP_REVANNCONTENT, crlDetails, X509_EXTENSIONS)
} ASN1_SEQUENCE_END(OSSL_CMP_REVANNCONTENT)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_REVANNCONTENT)

ASN1_SEQUENCE(OSSL_CMP_CHALLENGE) = {
    ASN1_OPT(OSSL_CMP_CHALLENGE, owf, X509_ALGOR),
    ASN1_SIMPLE(OSSL_CMP_CHALLENGE, witness, ASN1_OCTET_STRING),
    ASN1_SIMPLE(OSSL_CMP_CHALLENGE, challenge, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(OSSL_CMP_CHALLENGE)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_CHALLENGE)

ASN1_ITEM_TEMPLATE(OSSL_CMP_POPODECKEYCHALLCONTENT) =
    ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SEQUENCE_OF, 0,
                          OSSL_CMP_POPODECKEYCHALLCONTENT, OSSL_CMP_CHALLENGE)
ASN1_ITEM_TEMPLATE_END(OSSL_CMP_POPODECKEYCHALLCONTENT)

ASN1_ITEM_TEMPLATE(OSSL_CMP_POPODECKEYRESPCONTENT) =
    ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SEQUENCE_OF, 0,
                          OSSL_CMP_POPODECKEYRESPCONTENT, ASN1_INTEGER)
ASN1_ITEM_TEMPLATE_END(OSSL_CMP_POPODECKEYRESPCONTENT)

ASN1_SEQUENCE(OSSL_CMP_CAKEYUPDANNCONTENT) = {
    /* OSSL_CMP_CMPCERTIFICATE is effectively X509 so it is used directly */
    ASN1_SIMPLE(OSSL_CMP_CAKEYUPDANNCONTENT, oldWithNew, X509),
    /* OSSL_CMP_CMPCERTIFICATE is effectively X509 so it is used directly */
    ASN1_SIMPLE(OSSL_CMP_CAKEYUPDANNCONTENT, newWithOld, X509),
    /* OSSL_CMP_CMPCERTIFICATE is effectively X509 so it is used directly */
    ASN1_SIMPLE(OSSL_CMP_CAKEYUPDANNCONTENT, newWithNew, X509)
} ASN1_SEQUENCE_END(OSSL_CMP_CAKEYUPDANNCONTENT)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_CAKEYUPDANNCONTENT)

ASN1_SEQUENCE(OSSL_CMP_ERRORMSGCONTENT) = {
    ASN1_SIMPLE(OSSL_CMP_ERRORMSGCONTENT, pKIStatusInfo, OSSL_CMP_PKISI),
    ASN1_OPT(OSSL_CMP_ERRORMSGCONTENT, errorCode, ASN1_INTEGER),
    /*
     * OSSL_CMP_PKIFREETEXT is effectively a sequence of ASN1_UTF8STRING
     * so it is used directly
     *
     */
    ASN1_SEQUENCE_OF_OPT(OSSL_CMP_ERRORMSGCONTENT, errorDetails,
                         ASN1_UTF8STRING)
} ASN1_SEQUENCE_END(OSSL_CMP_ERRORMSGCONTENT)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_ERRORMSGCONTENT)

ASN1_ADB_TEMPLATE(infotypeandvalue_default) = ASN1_OPT(OSSL_CMP_ITAV,
                                                       infoValue.other,
                                                       ASN1_ANY);
/* ITAV means InfoTypeAndValue */
ASN1_ADB(OSSL_CMP_ITAV) = {
    /* OSSL_CMP_CMPCERTIFICATE is effectively X509 so it is used directly */
    ADB_ENTRY(NID_id_it_caProtEncCert, ASN1_OPT(OSSL_CMP_ITAV,
                                                infoValue.caProtEncCert, X509)),
    ADB_ENTRY(NID_id_it_signKeyPairTypes,
              ASN1_SEQUENCE_OF_OPT(OSSL_CMP_ITAV,
                                   infoValue.signKeyPairTypes, X509_ALGOR)),
    ADB_ENTRY(NID_id_it_encKeyPairTypes,
              ASN1_SEQUENCE_OF_OPT(OSSL_CMP_ITAV,
                                   infoValue.encKeyPairTypes, X509_ALGOR)),
    ADB_ENTRY(NID_id_it_preferredSymmAlg,
              ASN1_OPT(OSSL_CMP_ITAV, infoValue.preferredSymmAlg,
                       X509_ALGOR)),
    ADB_ENTRY(NID_id_it_caKeyUpdateInfo,
              ASN1_OPT(OSSL_CMP_ITAV, infoValue.caKeyUpdateInfo,
                       OSSL_CMP_CAKEYUPDANNCONTENT)),
    ADB_ENTRY(NID_id_it_currentCRL,
              ASN1_OPT(OSSL_CMP_ITAV, infoValue.currentCRL, X509_CRL)),
    ADB_ENTRY(NID_id_it_unsupportedOIDs,
              ASN1_SEQUENCE_OF_OPT(OSSL_CMP_ITAV,
                                   infoValue.unsupportedOIDs, ASN1_OBJECT)),
    ADB_ENTRY(NID_id_it_keyPairParamReq,
              ASN1_OPT(OSSL_CMP_ITAV, infoValue.keyPairParamReq,
                       ASN1_OBJECT)),
    ADB_ENTRY(NID_id_it_keyPairParamRep,
              ASN1_OPT(OSSL_CMP_ITAV, infoValue.keyPairParamRep,
                       X509_ALGOR)),
    ADB_ENTRY(NID_id_it_revPassphrase,
              ASN1_OPT(OSSL_CMP_ITAV, infoValue.revPassphrase,
                       OSSL_CRMF_ENCRYPTEDVALUE)),
    ADB_ENTRY(NID_id_it_implicitConfirm,
              ASN1_OPT(OSSL_CMP_ITAV, infoValue.implicitConfirm,
                       ASN1_NULL)),
    ADB_ENTRY(NID_id_it_confirmWaitTime,
              ASN1_OPT(OSSL_CMP_ITAV, infoValue.confirmWaitTime,
                       ASN1_GENERALIZEDTIME)),
    ADB_ENTRY(NID_id_it_origPKIMessage,
              ASN1_OPT(OSSL_CMP_ITAV, infoValue.origPKIMessage,
                       OSSL_CMP_MSGS)),
    ADB_ENTRY(NID_id_it_suppLangTags,
              ASN1_SEQUENCE_OF_OPT(OSSL_CMP_ITAV, infoValue.suppLangTagsValue,
                                   ASN1_UTF8STRING)),
    ADB_ENTRY(NID_id_it_caCerts,
              ASN1_SEQUENCE_OF_OPT(OSSL_CMP_ITAV, infoValue.caCerts, X509)),
    ADB_ENTRY(NID_id_it_rootCaCert,
              ASN1_OPT(OSSL_CMP_ITAV, infoValue.rootCaCert, X509)),
    ADB_ENTRY(NID_id_it_rootCaKeyUpdate,
              ASN1_OPT(OSSL_CMP_ITAV, infoValue.rootCaKeyUpdate,
                       OSSL_CMP_ROOTCAKEYUPDATE)),
} ASN1_ADB_END(OSSL_CMP_ITAV, 0, infoType, 0,
               &infotypeandvalue_default_tt, NULL);

ASN1_SEQUENCE(OSSL_CMP_ITAV) = {
    ASN1_SIMPLE(OSSL_CMP_ITAV, infoType, ASN1_OBJECT),
    ASN1_ADB_OBJECT(OSSL_CMP_ITAV)
} ASN1_SEQUENCE_END(OSSL_CMP_ITAV)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_ITAV)
IMPLEMENT_ASN1_DUP_FUNCTION(OSSL_CMP_ITAV)

ASN1_SEQUENCE(OSSL_CMP_ROOTCAKEYUPDATE) = {
    /* OSSL_CMP_CMPCERTIFICATE is effectively X509 so it is used directly */
    ASN1_SIMPLE(OSSL_CMP_ROOTCAKEYUPDATE, newWithNew, X509),
    ASN1_EXP_OPT(OSSL_CMP_ROOTCAKEYUPDATE, newWithOld, X509, 0),
    ASN1_EXP_OPT(OSSL_CMP_ROOTCAKEYUPDATE, oldWithNew, X509, 1)
} ASN1_SEQUENCE_END(OSSL_CMP_ROOTCAKEYUPDATE)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_ROOTCAKEYUPDATE)

OSSL_CMP_ITAV *OSSL_CMP_ITAV_create(ASN1_OBJECT *type, ASN1_TYPE *value)
{
    OSSL_CMP_ITAV *itav;

    if (type == NULL || (itav = OSSL_CMP_ITAV_new()) == NULL)
        return NULL;
    OSSL_CMP_ITAV_set0(itav, type, value);
    return itav;
}

void OSSL_CMP_ITAV_set0(OSSL_CMP_ITAV *itav, ASN1_OBJECT *type,
                        ASN1_TYPE *value)
{
    itav->infoType = type;
    itav->infoValue.other = value;
}

ASN1_OBJECT *OSSL_CMP_ITAV_get0_type(const OSSL_CMP_ITAV *itav)
{
    if (itav == NULL)
        return NULL;
    return itav->infoType;
}

ASN1_TYPE *OSSL_CMP_ITAV_get0_value(const OSSL_CMP_ITAV *itav)
{
    if (itav == NULL)
        return NULL;
    return itav->infoValue.other;
}

int OSSL_CMP_ITAV_push0_stack_item(STACK_OF(OSSL_CMP_ITAV) **itav_sk_p,
                                   OSSL_CMP_ITAV *itav)
{
    int created = 0;

    if (itav_sk_p == NULL || itav == NULL) {
        ERR_raise(ERR_LIB_CMP, CMP_R_NULL_ARGUMENT);
        goto err;
    }

    if (*itav_sk_p == NULL) {
        if ((*itav_sk_p = sk_OSSL_CMP_ITAV_new_null()) == NULL)
            goto err;
        created = 1;
    }
    if (!sk_OSSL_CMP_ITAV_push(*itav_sk_p, itav))
        goto err;
    return 1;

 err:
    if (created != 0) {
        sk_OSSL_CMP_ITAV_free(*itav_sk_p);
        *itav_sk_p = NULL;
    }
    return 0;
}

OSSL_CMP_ITAV *OSSL_CMP_ITAV_new_caCerts(const STACK_OF(X509) *caCerts)
{
    OSSL_CMP_ITAV *itav = OSSL_CMP_ITAV_new();

    if (itav == NULL)
        return NULL;
    if (sk_X509_num(caCerts) > 0
        && (itav->infoValue.caCerts =
            sk_X509_deep_copy(caCerts, X509_dup, X509_free)) == NULL) {
        OSSL_CMP_ITAV_free(itav);
        return NULL;
    }
    itav->infoType = OBJ_nid2obj(NID_id_it_caCerts);
    return itav;
}

int OSSL_CMP_ITAV_get0_caCerts(const OSSL_CMP_ITAV *itav, STACK_OF(X509) **out)
{
    if (itav == NULL || out == NULL) {
        ERR_raise(ERR_LIB_CMP, ERR_R_PASSED_NULL_PARAMETER);
        return 0;
    }
    if (OBJ_obj2nid(itav->infoType) != NID_id_it_caCerts) {
        ERR_raise(ERR_LIB_CMP, ERR_R_PASSED_INVALID_ARGUMENT);
        return 0;
    }
    *out = sk_X509_num(itav->infoValue.caCerts) > 0
        ? itav->infoValue.caCerts : NULL;
    return 1;
}

OSSL_CMP_ITAV *OSSL_CMP_ITAV_new_rootCaCert(const X509 *rootCaCert)
{
    OSSL_CMP_ITAV *itav = OSSL_CMP_ITAV_new();

    if (itav == NULL)
        return NULL;
    if (rootCaCert != NULL
            && (itav->infoValue.rootCaCert = X509_dup(rootCaCert)) == NULL) {
        OSSL_CMP_ITAV_free(itav);
        return NULL;
    }
    itav->infoType = OBJ_nid2obj(NID_id_it_rootCaCert);
    return itav;
}

int OSSL_CMP_ITAV_get0_rootCaCert(const OSSL_CMP_ITAV *itav, X509 **out)
{
    if (itav == NULL || out == NULL) {
        ERR_raise(ERR_LIB_CMP, ERR_R_PASSED_NULL_PARAMETER);
        return 0;
    }
    if (OBJ_obj2nid(itav->infoType) != NID_id_it_rootCaCert) {
        ERR_raise(ERR_LIB_CMP, ERR_R_PASSED_INVALID_ARGUMENT);
        return 0;
    }
    *out = itav->infoValue.rootCaCert;
    return 1;
}
OSSL_CMP_ITAV *OSSL_CMP_ITAV_new_rootCaKeyUpdate(const X509 *newWithNew,
                                                 const X509 *newWithOld,
                                                 const X509 *oldWithNew)
{
    OSSL_CMP_ITAV *itav;
    OSSL_CMP_ROOTCAKEYUPDATE *upd = OSSL_CMP_ROOTCAKEYUPDATE_new();

    if (upd == NULL)
        return NULL;
    if (newWithNew != NULL && (upd->newWithNew = X509_dup(newWithNew)) == NULL)
        goto err;
    if (newWithOld != NULL && (upd->newWithOld = X509_dup(newWithOld)) == NULL)
        goto err;
    if (oldWithNew != NULL && (upd->oldWithNew = X509_dup(oldWithNew)) == NULL)
        goto err;
    if ((itav = OSSL_CMP_ITAV_new()) == NULL)
        goto err;
    itav->infoType = OBJ_nid2obj(NID_id_it_rootCaKeyUpdate);
    itav->infoValue.rootCaKeyUpdate = upd;
    return itav;

    err:
    OSSL_CMP_ROOTCAKEYUPDATE_free(upd);
    return NULL;
}

int OSSL_CMP_ITAV_get0_rootCaKeyUpdate(const OSSL_CMP_ITAV *itav,
                                       X509 **newWithNew,
                                       X509 **newWithOld,
                                       X509 **oldWithNew)
{
    OSSL_CMP_ROOTCAKEYUPDATE *upd;

    if (itav == NULL || newWithNew == NULL) {
        ERR_raise(ERR_LIB_CMP, ERR_R_PASSED_NULL_PARAMETER);
        return 0;
    }
    if (OBJ_obj2nid(itav->infoType) != NID_id_it_rootCaKeyUpdate) {
        ERR_raise(ERR_LIB_CMP, ERR_R_PASSED_INVALID_ARGUMENT);
        return 0;
    }
    upd = itav->infoValue.rootCaKeyUpdate;
    *newWithNew = upd->newWithNew;
    if (newWithOld != NULL)
        *newWithOld = upd->newWithOld;
    if (oldWithNew != NULL)
        *oldWithNew = upd->oldWithNew;
    return 1;
}

/* get ASN.1 encoded integer, return -2 on error; -1 is valid for certReqId */
int ossl_cmp_asn1_get_int(const ASN1_INTEGER *a)
{
    int64_t res;

    if (!ASN1_INTEGER_get_int64(&res, a)) {
        ERR_raise(ERR_LIB_CMP, ASN1_R_INVALID_NUMBER);
        return -2;
    }
    if (res < INT_MIN) {
        ERR_raise(ERR_LIB_CMP, ASN1_R_TOO_SMALL);
        return -2;
    }
    if (res > INT_MAX) {
        ERR_raise(ERR_LIB_CMP, ASN1_R_TOO_LARGE);
        return -2;
    }
    return (int)res;
}

static int ossl_cmp_msg_cb(int operation, ASN1_VALUE **pval,
                           const ASN1_ITEM *it, void *exarg)
{
    OSSL_CMP_MSG *msg = (OSSL_CMP_MSG *)*pval;

    switch (operation) {
    case ASN1_OP_FREE_POST:
        OPENSSL_free(msg->propq);
        break;

    case ASN1_OP_DUP_POST:
        {
            OSSL_CMP_MSG *old = exarg;

            if (!ossl_cmp_msg_set0_libctx(msg, old->libctx, old->propq))
                return 0;
        }
        break;
    case ASN1_OP_GET0_LIBCTX:
        {
            OSSL_LIB_CTX **libctx = exarg;

            *libctx = msg->libctx;
        }
        break;
    case ASN1_OP_GET0_PROPQ:
        {
            const char **propq = exarg;

            *propq = msg->propq;
        }
        break;
    default:
        break;
    }

    return 1;
}

ASN1_CHOICE(OSSL_CMP_CERTORENCCERT) = {
    /* OSSL_CMP_CMPCERTIFICATE is effectively X509 so it is used directly */
    ASN1_EXP(OSSL_CMP_CERTORENCCERT, value.certificate, X509, 0),
    ASN1_EXP(OSSL_CMP_CERTORENCCERT, value.encryptedCert,
             OSSL_CRMF_ENCRYPTEDVALUE, 1),
} ASN1_CHOICE_END(OSSL_CMP_CERTORENCCERT)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_CERTORENCCERT)

ASN1_SEQUENCE(OSSL_CMP_CERTIFIEDKEYPAIR) = {
    ASN1_SIMPLE(OSSL_CMP_CERTIFIEDKEYPAIR, certOrEncCert,
                OSSL_CMP_CERTORENCCERT),
    ASN1_EXP_OPT(OSSL_CMP_CERTIFIEDKEYPAIR, privateKey,
                 OSSL_CRMF_ENCRYPTEDVALUE, 0),
    ASN1_EXP_OPT(OSSL_CMP_CERTIFIEDKEYPAIR, publicationInfo,
                 OSSL_CRMF_PKIPUBLICATIONINFO, 1)
} ASN1_SEQUENCE_END(OSSL_CMP_CERTIFIEDKEYPAIR)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_CERTIFIEDKEYPAIR)

ASN1_SEQUENCE(OSSL_CMP_REVDETAILS) = {
    ASN1_SIMPLE(OSSL_CMP_REVDETAILS, certDetails, OSSL_CRMF_CERTTEMPLATE),
    ASN1_OPT(OSSL_CMP_REVDETAILS, crlEntryDetails, X509_EXTENSIONS)
} ASN1_SEQUENCE_END(OSSL_CMP_REVDETAILS)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_REVDETAILS)

ASN1_ITEM_TEMPLATE(OSSL_CMP_REVREQCONTENT) =
    ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SEQUENCE_OF, 0, OSSL_CMP_REVREQCONTENT,
                          OSSL_CMP_REVDETAILS)
ASN1_ITEM_TEMPLATE_END(OSSL_CMP_REVREQCONTENT)

ASN1_SEQUENCE(OSSL_CMP_REVREPCONTENT) = {
    ASN1_SEQUENCE_OF(OSSL_CMP_REVREPCONTENT, status, OSSL_CMP_PKISI),
    ASN1_EXP_SEQUENCE_OF_OPT(OSSL_CMP_REVREPCONTENT, revCerts, OSSL_CRMF_CERTID,
                             0),
    ASN1_EXP_SEQUENCE_OF_OPT(OSSL_CMP_REVREPCONTENT, crls, X509_CRL, 1)
} ASN1_SEQUENCE_END(OSSL_CMP_REVREPCONTENT)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_REVREPCONTENT)

ASN1_SEQUENCE(OSSL_CMP_KEYRECREPCONTENT) = {
    ASN1_SIMPLE(OSSL_CMP_KEYRECREPCONTENT, status, OSSL_CMP_PKISI),
    ASN1_EXP_OPT(OSSL_CMP_KEYRECREPCONTENT, newSigCert, X509, 0),
    ASN1_EXP_SEQUENCE_OF_OPT(OSSL_CMP_KEYRECREPCONTENT, caCerts, X509, 1),
    ASN1_EXP_SEQUENCE_OF_OPT(OSSL_CMP_KEYRECREPCONTENT, keyPairHist,
                             OSSL_CMP_CERTIFIEDKEYPAIR, 2)
} ASN1_SEQUENCE_END(OSSL_CMP_KEYRECREPCONTENT)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_KEYRECREPCONTENT)

ASN1_ITEM_TEMPLATE(OSSL_CMP_PKISTATUS) =
    ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_UNIVERSAL, 0, status, ASN1_INTEGER)
ASN1_ITEM_TEMPLATE_END(OSSL_CMP_PKISTATUS)

ASN1_SEQUENCE(OSSL_CMP_PKISI) = {
    ASN1_SIMPLE(OSSL_CMP_PKISI, status, OSSL_CMP_PKISTATUS),
    /*
     * CMP_PKIFREETEXT is effectively a sequence of ASN1_UTF8STRING
     * so it is used directly
     */
    ASN1_SEQUENCE_OF_OPT(OSSL_CMP_PKISI, statusString, ASN1_UTF8STRING),
    /*
     * OSSL_CMP_PKIFAILUREINFO is effectively ASN1_BIT_STRING so used directly
     */
    ASN1_OPT(OSSL_CMP_PKISI, failInfo, ASN1_BIT_STRING)
} ASN1_SEQUENCE_END(OSSL_CMP_PKISI)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_PKISI)
IMPLEMENT_ASN1_DUP_FUNCTION(OSSL_CMP_PKISI)

ASN1_SEQUENCE(OSSL_CMP_CERTSTATUS) = {
    ASN1_SIMPLE(OSSL_CMP_CERTSTATUS, certHash, ASN1_OCTET_STRING),
    ASN1_SIMPLE(OSSL_CMP_CERTSTATUS, certReqId, ASN1_INTEGER),
    ASN1_OPT(OSSL_CMP_CERTSTATUS, statusInfo, OSSL_CMP_PKISI),
    ASN1_EXP_OPT(OSSL_CMP_CERTSTATUS, hashAlg, X509_ALGOR, 0)
} ASN1_SEQUENCE_END(OSSL_CMP_CERTSTATUS)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_CERTSTATUS)

ASN1_ITEM_TEMPLATE(OSSL_CMP_CERTCONFIRMCONTENT) =
    ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SEQUENCE_OF, 0, OSSL_CMP_CERTCONFIRMCONTENT,
                          OSSL_CMP_CERTSTATUS)
ASN1_ITEM_TEMPLATE_END(OSSL_CMP_CERTCONFIRMCONTENT)

ASN1_SEQUENCE(OSSL_CMP_CERTRESPONSE) = {
    ASN1_SIMPLE(OSSL_CMP_CERTRESPONSE, certReqId, ASN1_INTEGER),
    ASN1_SIMPLE(OSSL_CMP_CERTRESPONSE, status, OSSL_CMP_PKISI),
    ASN1_OPT(OSSL_CMP_CERTRESPONSE, certifiedKeyPair,
             OSSL_CMP_CERTIFIEDKEYPAIR),
    ASN1_OPT(OSSL_CMP_CERTRESPONSE, rspInfo, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(OSSL_CMP_CERTRESPONSE)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_CERTRESPONSE)

ASN1_SEQUENCE(OSSL_CMP_POLLREQ) = {
    ASN1_SIMPLE(OSSL_CMP_POLLREQ, certReqId, ASN1_INTEGER)
} ASN1_SEQUENCE_END(OSSL_CMP_POLLREQ)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_POLLREQ)

ASN1_ITEM_TEMPLATE(OSSL_CMP_POLLREQCONTENT) =
    ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SEQUENCE_OF, 0, OSSL_CMP_POLLREQCONTENT,
                          OSSL_CMP_POLLREQ)
ASN1_ITEM_TEMPLATE_END(OSSL_CMP_POLLREQCONTENT)

ASN1_SEQUENCE(OSSL_CMP_POLLREP) = {
    ASN1_SIMPLE(OSSL_CMP_POLLREP, certReqId, ASN1_INTEGER),
    ASN1_SIMPLE(OSSL_CMP_POLLREP, checkAfter, ASN1_INTEGER),
    ASN1_SEQUENCE_OF_OPT(OSSL_CMP_POLLREP, reason, ASN1_UTF8STRING),
} ASN1_SEQUENCE_END(OSSL_CMP_POLLREP)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_POLLREP)

ASN1_ITEM_TEMPLATE(OSSL_CMP_POLLREPCONTENT) =
    ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SEQUENCE_OF, 0,
                          OSSL_CMP_POLLREPCONTENT,
                          OSSL_CMP_POLLREP)
ASN1_ITEM_TEMPLATE_END(OSSL_CMP_POLLREPCONTENT)

ASN1_SEQUENCE(OSSL_CMP_CERTREPMESSAGE) = {
    /* OSSL_CMP_CMPCERTIFICATE is effectively X509 so it is used directly */
    ASN1_EXP_SEQUENCE_OF_OPT(OSSL_CMP_CERTREPMESSAGE, caPubs, X509, 1),
    ASN1_SEQUENCE_OF(OSSL_CMP_CERTREPMESSAGE, response, OSSL_CMP_CERTRESPONSE)
} ASN1_SEQUENCE_END(OSSL_CMP_CERTREPMESSAGE)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_CERTREPMESSAGE)

ASN1_ITEM_TEMPLATE(OSSL_CMP_GENMSGCONTENT) =
    ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SEQUENCE_OF, 0, OSSL_CMP_GENMSGCONTENT,
                          OSSL_CMP_ITAV)
ASN1_ITEM_TEMPLATE_END(OSSL_CMP_GENMSGCONTENT)

ASN1_ITEM_TEMPLATE(OSSL_CMP_GENREPCONTENT) =
    ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SEQUENCE_OF, 0, OSSL_CMP_GENREPCONTENT,
                          OSSL_CMP_ITAV)
ASN1_ITEM_TEMPLATE_END(OSSL_CMP_GENREPCONTENT)

ASN1_ITEM_TEMPLATE(OSSL_CMP_CRLANNCONTENT) =
    ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SEQUENCE_OF, 0,
                          OSSL_CMP_CRLANNCONTENT, X509_CRL)
ASN1_ITEM_TEMPLATE_END(OSSL_CMP_CRLANNCONTENT)

ASN1_CHOICE(OSSL_CMP_PKIBODY) = {
    ASN1_EXP(OSSL_CMP_PKIBODY, value.ir, OSSL_CRMF_MSGS, 0),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.ip, OSSL_CMP_CERTREPMESSAGE, 1),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.cr, OSSL_CRMF_MSGS, 2),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.cp, OSSL_CMP_CERTREPMESSAGE, 3),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.p10cr, X509_REQ, 4),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.popdecc,
             OSSL_CMP_POPODECKEYCHALLCONTENT, 5),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.popdecr,
             OSSL_CMP_POPODECKEYRESPCONTENT, 6),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.kur, OSSL_CRMF_MSGS, 7),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.kup, OSSL_CMP_CERTREPMESSAGE, 8),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.krr, OSSL_CRMF_MSGS, 9),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.krp, OSSL_CMP_KEYRECREPCONTENT, 10),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.rr, OSSL_CMP_REVREQCONTENT, 11),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.rp, OSSL_CMP_REVREPCONTENT, 12),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.ccr, OSSL_CRMF_MSGS, 13),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.ccp, OSSL_CMP_CERTREPMESSAGE, 14),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.ckuann, OSSL_CMP_CAKEYUPDANNCONTENT, 15),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.cann, X509, 16),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.rann, OSSL_CMP_REVANNCONTENT, 17),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.crlann, OSSL_CMP_CRLANNCONTENT, 18),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.pkiconf, ASN1_ANY, 19),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.nested, OSSL_CMP_MSGS, 20),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.genm, OSSL_CMP_GENMSGCONTENT, 21),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.genp, OSSL_CMP_GENREPCONTENT, 22),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.error, OSSL_CMP_ERRORMSGCONTENT, 23),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.certConf, OSSL_CMP_CERTCONFIRMCONTENT, 24),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.pollReq, OSSL_CMP_POLLREQCONTENT, 25),
    ASN1_EXP(OSSL_CMP_PKIBODY, value.pollRep, OSSL_CMP_POLLREPCONTENT, 26),
} ASN1_CHOICE_END(OSSL_CMP_PKIBODY)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_PKIBODY)

ASN1_SEQUENCE(OSSL_CMP_PKIHEADER) = {
    ASN1_SIMPLE(OSSL_CMP_PKIHEADER, pvno, ASN1_INTEGER),
    ASN1_SIMPLE(OSSL_CMP_PKIHEADER, sender, GENERAL_NAME),
    ASN1_SIMPLE(OSSL_CMP_PKIHEADER, recipient, GENERAL_NAME),
    ASN1_EXP_OPT(OSSL_CMP_PKIHEADER, messageTime, ASN1_GENERALIZEDTIME, 0),
    ASN1_EXP_OPT(OSSL_CMP_PKIHEADER, protectionAlg, X509_ALGOR, 1),
    ASN1_EXP_OPT(OSSL_CMP_PKIHEADER, senderKID, ASN1_OCTET_STRING, 2),
    ASN1_EXP_OPT(OSSL_CMP_PKIHEADER, recipKID, ASN1_OCTET_STRING, 3),
    ASN1_EXP_OPT(OSSL_CMP_PKIHEADER, transactionID, ASN1_OCTET_STRING, 4),
    ASN1_EXP_OPT(OSSL_CMP_PKIHEADER, senderNonce, ASN1_OCTET_STRING, 5),
    ASN1_EXP_OPT(OSSL_CMP_PKIHEADER, recipNonce, ASN1_OCTET_STRING, 6),
    /*
     * OSSL_CMP_PKIFREETEXT is effectively a sequence of ASN1_UTF8STRING
     * so it is used directly
     */
    ASN1_EXP_SEQUENCE_OF_OPT(OSSL_CMP_PKIHEADER, freeText, ASN1_UTF8STRING, 7),
    ASN1_EXP_SEQUENCE_OF_OPT(OSSL_CMP_PKIHEADER, generalInfo,
                             OSSL_CMP_ITAV, 8)
} ASN1_SEQUENCE_END(OSSL_CMP_PKIHEADER)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_PKIHEADER)

ASN1_SEQUENCE(OSSL_CMP_PROTECTEDPART) = {
    ASN1_SIMPLE(OSSL_CMP_MSG, header, OSSL_CMP_PKIHEADER),
    ASN1_SIMPLE(OSSL_CMP_MSG, body, OSSL_CMP_PKIBODY)
} ASN1_SEQUENCE_END(OSSL_CMP_PROTECTEDPART)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CMP_PROTECTEDPART)

ASN1_SEQUENCE_cb(OSSL_CMP_MSG, ossl_cmp_msg_cb) = {
    ASN1_SIMPLE(OSSL_CMP_MSG, header, OSSL_CMP_PKIHEADER),
    ASN1_SIMPLE(OSSL_CMP_MSG, body, OSSL_CMP_PKIBODY),
    ASN1_EXP_OPT(OSSL_CMP_MSG, protection, ASN1_BIT_STRING, 0),
    /* OSSL_CMP_CMPCERTIFICATE is effectively X509 so it is used directly */
    ASN1_EXP_SEQUENCE_OF_OPT(OSSL_CMP_MSG, extraCerts, X509, 1)
} ASN1_SEQUENCE_END_cb(OSSL_CMP_MSG, OSSL_CMP_MSG)
IMPLEMENT_ASN1_DUP_FUNCTION(OSSL_CMP_MSG)

ASN1_ITEM_TEMPLATE(OSSL_CMP_MSGS) =
    ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SEQUENCE_OF, 0, OSSL_CMP_MSGS,
                          OSSL_CMP_MSG)
ASN1_ITEM_TEMPLATE_END(OSSL_CMP_MSGS)
