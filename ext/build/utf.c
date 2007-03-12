/*
 * utf.c:  UTF-8 conversion routines
 *
 * ====================================================================
 * Copyright (c) 2000-2007 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://subversion.tigris.org/.
 * ====================================================================
 */



#include <string.h>
#include <assert.h>

#include <apr_strings.h>
#include <apr_lib.h>
#include <apr_xlate.h>

#include "svn_string.h"
#include "svn_error.h"
#include "svn_pools.h"
#include "svn_ctype.h"
#include "svn_utf.h"
#include "utf_impl.h"
#include "svn_private_config.h"



#define SVN_UTF_NTOU_XLATE_HANDLE "svn-utf-ntou-xlate-handle"
#define SVN_UTF_UTON_XLATE_HANDLE "svn-utf-uton-xlate-handle"

#ifndef AS400
#define SVN_APR_UTF8_CHARSET "UTF-8"
#else
#define SVN_APR_UTF8_CHARSET (const char*)1208
#endif

#if !APR_HAS_XLATE
#if APR_HAS_THREADS
static apr_thread_mutex_t *xlate_handle_mutex = NULL;
#endif

/* The xlate handle cache is a global hash table with linked lists of xlate
 * handles.  In multi-threaded environments, a thread "borrows" an xlate
 * handle from the cache during a translation and puts it back afterwards.
 * This avoids holding a global lock for all translations.
 * If there is no handle for a particular key when needed, a new is
 * handle is created and put in the cache after use.
 * This means that there will be at most N handles open for a key, where N
 * is the number of simultanous handles in use for that key. */

typedef struct xlate_handle_node_t {
  apr_xlate_t *handle;
  /* FALSE if the handle is not valid, since its pool is being
     destroyed. */
  svn_boolean_t valid;
  /* The name of a char encoding or APR_LOCALE_CHARSET. */
  const char *frompage, *topage;
  struct xlate_handle_node_t *next;
} xlate_handle_node_t;

/* This maps const char * userdata_key strings to xlate_handle_node_t **
   handles to the first entry in the linked list of xlate handles.  We don't
   store the pointer to the list head directly in the hash table, since we
   remove/insert entries at the head in the list in the code below, and
   we can't use apr_hash_set() in each character translation because that
   function allocates memory in each call where the value is non-NULL.
   Since these allocations take place in a global pool, this would be a
   memory leak. */
static apr_hash_t *xlate_handle_hash = NULL;

/* Clean up the xlate handle cache. */
static apr_status_t
xlate_cleanup(void *arg)
{
  /* We set the cache variables to NULL so that translation works in other
     cleanup functions, even if it isn't cached then. */
#if APR_HAS_THREADS
  apr_thread_mutex_destroy(xlate_handle_mutex);
  xlate_handle_mutex = NULL;
#endif
  xlate_handle_hash = NULL;

  return APR_SUCCESS;
}

/* Set the handle of ARG to NULL. */
static apr_status_t
xlate_handle_node_cleanup(void *arg)
{
  xlate_handle_node_t *node = arg;

  node->valid = FALSE;
  return APR_SUCCESS;
}
#endif /* !APR_HAS_XLATE */

void
svn_utf_initialize(apr_pool_t *pool)
{
#if APR_HAS_XLATE
  apr_pool_t *subpool;
#if APR_HAS_THREADS
  apr_thread_mutex_t *mutex;
#endif

  if (!xlate_handle_hash)
    {
      /* We create our own subpool, which we protect with the mutex.
         We can't use the pool passed to us by the caller, since we will
         use it for xlate handle allocations, possibly in multiple threads,
         and pool allocation is not thread-safe. */
      subpool = svn_pool_create(pool);
#if APR_HAS_THREADS
      if (apr_thread_mutex_create(&mutex, APR_THREAD_MUTEX_DEFAULT, subpool)
          == APR_SUCCESS)
        xlate_handle_mutex = mutex;
      else
        return;
#endif
      
      xlate_handle_hash = apr_hash_make(subpool);
      apr_pool_cleanup_register(subpool, NULL, xlate_cleanup,
                                apr_pool_cleanup_null);
    }
#endif /* APR_HAS_XLATE */
}

#if APR_HAS_XLATE
/* Return a unique string key based on TOPAGE and FROMPAGE.  TOPAGE and
 * FROMPAGE can be any valid arguments of the same name to
 * apr_xlate_open().  Allocate the returned string in POOL. */
static const char*
get_xlate_key(const char *topage,
              const char *frompage,
              apr_pool_t *pool)
{
#ifndef AS400
  /* In the cases of SVN_APR_LOCALE_CHARSET and SVN_APR_DEFAULT_CHARSET
   * topage/frompage is really an int, not a valid string.  So generate a
   * unique key accordingly. */
  if (frompage == SVN_APR_LOCALE_CHARSET)
    frompage = "APR_LOCALE_CHARSET";
  else if (frompage == SVN_APR_DEFAULT_CHARSET)
    frompage = "APR_DEFAULT_CHARSET";

  if (topage == SVN_APR_LOCALE_CHARSET)
    topage = "APR_LOCALE_CHARSET";
  else if (topage == SVN_APR_DEFAULT_CHARSET)
    topage = "APR_DEFAULT_CHARSET";

  return apr_pstrcat(pool, "svn-utf-", frompage, "to", topage,
                     "-xlate-handle", NULL);
#else
  /* OS400 code pages are always ints. */
  return apr_psprintf(pool, "svn-utf-%dto%d-xlate-handle", (int)frompage,
                      (int)topage);
#endif
}

/* Set *RET to a handle node for converting from FROMPAGE to TOPAGE,
   creating the handle node if it doesn't exist in USERDATA_KEY.
   If a node is not cached and apr_xlate_open() returns APR_EINVAL or
   APR_ENOTIMPL, set (*RET)->handle to NULL.  If fail for any other
   reason, return the error.

   Allocate *RET and its xlate handle in POOL if svn_utf_initialize()
   hasn't been called or USERDATA_KEY is NULL.  Else, allocate them
   in the pool of xlate_handle_hash. */
static svn_error_t *
get_xlate_handle_node(xlate_handle_node_t **ret,
                      const char *topage, const char *frompage,
                      const char *userdata_key, apr_pool_t *pool)
{
  xlate_handle_node_t **old_node_p;
  xlate_handle_node_t *old_node = NULL;
  apr_status_t apr_err;
  apr_xlate_t *handle;
  svn_error_t *err = NULL;

  /* If we already have a handle, just return it. */
  if (userdata_key)
    {
      if (xlate_handle_hash)
        {
#if APR_HAS_THREADS
          apr_err = apr_thread_mutex_lock(xlate_handle_mutex);
          if (apr_err != APR_SUCCESS)
            return svn_error_create(apr_err, NULL,
                                    _("Can't lock charset translation mutex"));
#endif
          old_node_p = apr_hash_get(xlate_handle_hash, userdata_key,
                                    APR_HASH_KEY_STRING);
          if (old_node_p)
            old_node = *old_node_p;
          if (old_node)
            {
              /* Ensure that the handle is still valid. */
              if (old_node->valid)
                {
                  /* Remove from the list. */
                  *old_node_p = old_node->next;
                  old_node->next = NULL;
#if APR_HAS_THREADS
                  apr_err = apr_thread_mutex_unlock(xlate_handle_mutex);
                  if (apr_err != APR_SUCCESS)
                    return svn_error_create(apr_err, NULL,
                                            _("Can't unlock charset "
                                              "translation mutex"));
#endif
                  *ret = old_node;
                  return SVN_NO_ERROR;
                }
            }
        }
      else
        {
          void *p;
          /* We fall back on a per-pool cache instead. */
          apr_pool_userdata_get(&p, userdata_key, pool);
          old_node = p;
          /* Ensure that the handle is still valid. */
          if (old_node && old_node->valid)
            {
              *ret = old_node;
              return SVN_NO_ERROR;
            }
        }
    }

  /* Note that we still have the mutex locked (if it is initialized), so we
     can use the global pool for creating the new xlate handle. */

  /* The error handling doesn't support the following cases, since we don't
     use them currently.  Catch this here. */
#ifndef AS400
  /* On OS400 V5R4 with UTF support, APR_DEFAULT_CHARSET and
   * APR_LOCALE_CHARSET are both UTF-8 (CCSID 1208), so we won't get far
   * with this assert active. */
  assert(frompage != SVN_APR_DEFAULT_CHARSET
         && topage != SVN_APR_DEFAULT_CHARSET
         && (frompage != SVN_APR_LOCALE_CHARSET
             || topage != SVN_APR_LOCALE_CHARSET));
#endif

  /* Use the correct pool for creating the handle. */
  if (userdata_key && xlate_handle_hash)
    pool = apr_hash_pool_get(xlate_handle_hash);

  /* Try to create a handle. */
#ifndef AS400
  apr_err = apr_xlate_open(&handle, topage, frompage, pool);
#else
  apr_err = apr_xlate_open(&handle, (int)topage, (int)frompage, pool);
#endif

  if (APR_STATUS_IS_EINVAL(apr_err) || APR_STATUS_IS_ENOTIMPL(apr_err))
    handle = NULL;
  else if (apr_err != APR_SUCCESS)
    {
      const char *errstr;
      /* Can't use svn_error_wrap_apr here because it calls functions in
         this file, leading to infinite recursion. */
#ifndef AS400
      if (frompage == SVN_APR_LOCALE_CHARSET)
        errstr = apr_psprintf(pool,
                              _("Can't create a character converter from "
                                "native encoding to '%s'"), topage);
      else if (topage == SVN_APR_LOCALE_CHARSET)
        errstr = apr_psprintf(pool,
                              _("Can't create a character converter from "
                                "'%s' to native encoding"), frompage);
      else
        errstr = apr_psprintf(pool,
                              _("Can't create a character converter from "
                                "'%s' to '%s'"), frompage, topage);
#else
      /* Handle the error condition normally prevented by the assert
       * above. */
      errstr = apr_psprintf(pool,
                            _("Can't create a character converter from "
                              "'%i' to '%i'"), frompage, topage);
#endif
      err = svn_error_create(apr_err, NULL, errstr);
      goto cleanup;
    }

  /* Allocate and initialize the node. */
  *ret = apr_palloc(pool, sizeof(xlate_handle_node_t));
  (*ret)->handle = handle;
  (*ret)->valid = TRUE;
  (*ret)->frompage = ((frompage != SVN_APR_LOCALE_CHARSET)
                      ? apr_pstrdup(pool, frompage) : frompage);
  (*ret)->topage = ((topage != SVN_APR_LOCALE_CHARSET)
                    ? apr_pstrdup(pool, topage) : topage);
  (*ret)->next = NULL;

  /* If we are called from inside a pool cleanup handler, the just created
     xlate handle will be closed when that handler returns by a newly
     registered cleanup handler, however, the handle is still cached by us.
     To prevent this, we register a cleanup handler that will reset the valid
     flag of our node, so we don't use an invalid handle. */
  if (handle)
    apr_pool_cleanup_register(pool, *ret, xlate_handle_node_cleanup,
                              apr_pool_cleanup_null);

 cleanup:
  /* Don't need the lock anymore. */
#if APR_HAS_THREADS
  if (userdata_key && xlate_handle_hash)
    {
      apr_status_t unlock_err = apr_thread_mutex_unlock(xlate_handle_mutex);
      if (unlock_err != APR_SUCCESS)
        return svn_error_create(unlock_err, NULL,
                                _("Can't unlock charset translation mutex"));
    }
#endif

  return err;
}

/* Put back NODE into the xlate handle cache for use by other calls.
   If there is no global cache, store the handle in POOL.
   Ignore errors related to locking/unlocking the mutex.
   ### Mutex errors here are very weird. Should we handle them "correctly"
   ### even if that complicates error handling in the routines below? */
static void
put_xlate_handle_node(xlate_handle_node_t *node,
                      const char *userdata_key,
                      apr_pool_t *pool)
{
  assert(node->next == NULL);
  if (!userdata_key)
    return;
  if (xlate_handle_hash)
    {
      xlate_handle_node_t **node_p;
#if APR_HAS_THREADS
      if (apr_thread_mutex_lock(xlate_handle_mutex) != APR_SUCCESS)
        abort();
#endif
      node_p = apr_hash_get(xlate_handle_hash, userdata_key,
                            APR_HASH_KEY_STRING);
      if (node_p == NULL)
        {
          userdata_key = apr_pstrdup(apr_hash_pool_get(xlate_handle_hash),
                                     userdata_key);
          node_p = apr_palloc(apr_hash_pool_get(xlate_handle_hash),
                              sizeof(*node_p));
          *node_p = NULL;
          apr_hash_set(xlate_handle_hash, userdata_key,
                       APR_HASH_KEY_STRING, node_p);
        }
      node->next = *node_p;
      *node_p = node;
#if APR_HAS_THREADS
      if (apr_thread_mutex_unlock(xlate_handle_mutex) != APR_SUCCESS)
        abort();
#endif
    }
  else
    {
      /* Store it in the per-pool cache. */
      apr_pool_userdata_set(node, userdata_key, apr_pool_cleanup_null, pool);
    }
}

/* Return the apr_xlate handle for converting native characters to UTF-8. */
static svn_error_t *
get_ntou_xlate_handle_node(xlate_handle_node_t **ret, apr_pool_t *pool)
{
  return get_xlate_handle_node(ret, SVN_APR_UTF8_CHARSET,
                               SVN_APR_LOCALE_CHARSET,
                               SVN_UTF_NTOU_XLATE_HANDLE, pool);
}


/* Return the apr_xlate handle for converting UTF-8 to native characters.
   Create one if it doesn't exist.  If unable to find a handle, or
   unable to create one because apr_xlate_open returned APR_EINVAL, then
   set *RET to null and return SVN_NO_ERROR; if fail for some other
   reason, return error. */
static svn_error_t *
get_uton_xlate_handle_node(xlate_handle_node_t **ret, apr_pool_t *pool)
{
  return get_xlate_handle_node(ret, SVN_APR_LOCALE_CHARSET,
                               SVN_APR_UTF8_CHARSET,
                               SVN_UTF_UTON_XLATE_HANDLE, pool);
}
#endif /* APR_HAS_XLATE */

/* Copy LEN bytes of SRC, converting non-ASCII and zero bytes to ?\nnn
   sequences, allocating the result in POOL. */
static const char *
fuzzy_escape(const char *src, apr_size_t len, apr_pool_t *pool)
{
  const char *src_orig = src, *src_end = src + len;
  apr_size_t new_len = 0;
  char *new;
  const char *new_orig;

  /* First count how big a dest string we'll need. */
  while (src < src_end)
    {
      if (! svn_ctype_isascii(*src) || *src == '\0')
        new_len += 5;  /* 5 slots, for "?\XXX" */
      else
        new_len += 1;  /* one slot for the 7-bit char */

      src++;
    }

  /* Allocate that amount. */
  new = apr_palloc(pool, new_len + 1);

  new_orig = new;

  /* And fill it up. */
  while (src_orig < src_end)
    {
      if (! svn_ctype_isascii(*src_orig) || src_orig == '\0')
        {
          /* This is the same format as svn_xml_fuzzy_escape uses, but that
             function escapes different characters.  Please keep in sync!
             ### If we add another fuzzy escape somewhere, we should abstract
             ### this out to a common function. */
          sprintf(new, "?\\%03u", (unsigned char) *src_orig);
          new += 5;
        }
      else
        {
          *new = *src_orig;
          new += 1;
        }

      src_orig++;
    }

  *new = '\0';

  return new_orig;
}

#if APR_HAS_XLATE
/* Convert SRC_LENGTH bytes of SRC_DATA in NODE->handle, store the result
   in *DEST, which is allocated in POOL. */
static svn_error_t *
convert_to_stringbuf(xlate_handle_node_t *node,
                     const char *src_data,
                     apr_size_t src_length,
                     svn_stringbuf_t **dest,
                     apr_pool_t *pool)
{
  apr_size_t buflen = src_length * 2;
  apr_status_t apr_err;
  apr_size_t srclen = src_length;
  apr_size_t destlen = buflen;
  char *destbuf;

  /* Initialize *DEST to an empty stringbuf. */
  *dest = svn_stringbuf_create("", pool);
  destbuf = (*dest)->data;

  /* Not only does it not make sense to convert an empty string, but
     apr-iconv is quite unreasonable about not allowing that. */
  if (src_length == 0)
    return SVN_NO_ERROR;

  do 
    {
      /* A 1:2 ratio of input bytes to output bytes (as assigned above)
         should be enough for most translations, and if it turns out not
         to be enough, we'll grow the buffer again, sizing it based on a
         1:3 ratio of the remainder of the string.

         We also want to ensure that the output buffer always has at
         least 3 bytes spare so that we always have room to convert at
         least one character (we assume that no encoding uses more than
         three bytes for a character) */
      if (destlen < 3)
        buflen += (srclen * 3);

      /* Ensure that *DEST has sufficient storage for the translated
         result. */
      svn_stringbuf_ensure(*dest, buflen + 1);

      /* Update the destination buffer pointer to the first character
         after already-converted output. */
      destbuf = (*dest)->data + (*dest)->len;

      /* Set up state variables for xlate. */
      destlen = buflen - (*dest)->len;
      assert(destlen >= 3);

      /* Attempt the conversion. */
      apr_err = apr_xlate_conv_buffer(node->handle,
                                      src_data + (src_length - srclen), 
                                      &srclen,
                                      destbuf, 
                                      &destlen);

      /* Now, update the *DEST->len to track the amount of output data
         churned out so far from this loop. */
      (*dest)->len += ((buflen - (*dest)->len) - destlen);

    } while (! apr_err && srclen);

  /* If we exited the loop with an error, return the error. */
  if (apr_err)
    {
      const char *errstr;
      svn_error_t *err;

      /* Can't use svn_error_wrap_apr here because it calls functions in
         this file, leading to infinite recursion. */
#ifndef AS400
      if (node->frompage == SVN_APR_LOCALE_CHARSET)
        errstr = apr_psprintf
          (pool, _("Can't convert string from native encoding to '%s':"),
           node->topage);
      else if (node->topage == SVN_APR_LOCALE_CHARSET)
        errstr = apr_psprintf
          (pool, _("Can't convert string from '%s' to native encoding:"),
           node->frompage);
      else
        errstr = apr_psprintf
          (pool, _("Can't convert string from '%s' to '%s':"),
           node->frompage, node->topage);
#else
      /* On OS400 V5R4 every possible node->topage and node->frompage
       * *really* is an int. */
      errstr = apr_psprintf
        (pool, _("Can't convert string from CCSID '%i' to CCSID '%i'"),
         node->frompage, node->topage);
#endif
      err = svn_error_create(apr_err, NULL, fuzzy_escape(src_data,
                                                         src_length, pool));
      return svn_error_create(apr_err, err, errstr);
    }
  /* Else, exited due to success.  Trim the result buffer down to the
     right length. */
  (*dest)->data[(*dest)->len] = '\0';

  return SVN_NO_ERROR;
}
#endif /* APR_HAS_XLATE */

/* Return APR_EINVAL if the first LEN bytes of DATA contain anything
   other than seven-bit, non-control (except for whitespace) ASCII
   characters, finding the error pool from POOL.  Otherwise, return
   SVN_NO_ERROR. */
static svn_error_t *
check_non_ascii(const char *data, apr_size_t len, apr_pool_t *pool)
{
  const char *data_start = data;

  for (; len > 0; --len, data++)
    {
      if ((! apr_isascii(*data))
          || ((! apr_isspace(*data))
              && apr_iscntrl(*data)))
        {
          /* Show the printable part of the data, followed by the
             decimal code of the questionable character.  Because if a
             user ever gets this error, she's going to have to spend
             time tracking down the non-ASCII data, so we want to help
             as much as possible.  And yes, we just call the unsafe
             data "non-ASCII", even though the actual constraint is
             somewhat more complex than that. */ 

          if (data - data_start)
            {
              const char *error_data
                = apr_pstrndup(pool, data_start, (data - data_start));

              return svn_error_createf
                (APR_EINVAL, NULL,
                 _("Safe data '%s' was followed by non-ASCII byte %d: "
                   "unable to convert to/from UTF-8"),
                 error_data, *((const unsigned char *) data));
            }
          else
            {
              return svn_error_createf
                (APR_EINVAL, NULL,
                 _("Non-ASCII character (code %d) detected, "
                   "and unable to convert to/from UTF-8"),
                 *((const unsigned char *) data));
            }
        }
    }

  return SVN_NO_ERROR;
}

/* Construct an error with a suitable message to describe the invalid UTF-8
 * sequence DATA of length LEN (which may have embedded NULLs).  We can't
 * simply print the data, almost by definition we don't really know how it
 * is encoded.
 */
static svn_error_t *
invalid_utf8(const char *data, apr_size_t len, apr_pool_t *pool)
{
  const char *last = svn_utf__last_valid(data, len);
  const char *valid_txt = "", *invalid_txt = "";
  int i, valid, invalid;

  /* We will display at most 24 valid octets (this may split a leading
     multi-byte character) as that should fit on one 80 character line. */
  valid = last - data;
  if (valid > 24)
    valid = 24;
  for (i = 0; i < valid; ++i)
    valid_txt = apr_pstrcat(pool, valid_txt,
                            apr_psprintf(pool, " %02x",
                                         (unsigned char)last[i-valid]), NULL);

  /* 4 invalid octets will guarantee that the faulty octet is displayed */
  invalid = data + len - last;
  if (invalid > 4)
    invalid = 4;
  for (i = 0; i < invalid; ++i)
    invalid_txt = apr_pstrcat(pool, invalid_txt,
                              apr_psprintf(pool, " %02x",
                                           (unsigned char)last[i]), NULL);

  return svn_error_createf(APR_EINVAL, NULL,
                           _("Valid UTF-8 data\n(hex:%s)\n"
                             "followed by invalid UTF-8 sequence\n(hex:%s)"),
                           valid_txt, invalid_txt);
}

/* Verify that the sequence DATA of length LEN is valid UTF-8 */
static svn_error_t *
check_utf8(const char *data, apr_size_t len, apr_pool_t *pool)
{
  if (! svn_utf__is_valid(data, len))
    return invalid_utf8(data, len, pool);
  return SVN_NO_ERROR;
}

/* Verify that the NULL terminated sequence DATA is valid UTF-8 */
static svn_error_t *
check_cstring_utf8(const char *data, apr_pool_t *pool)
{

  if (! svn_utf__cstring_is_valid(data))
    return invalid_utf8(data, strlen(data), pool);
  return SVN_NO_ERROR;
}


svn_error_t *
svn_utf_stringbuf_to_utf8(svn_stringbuf_t **dest,
                          const svn_stringbuf_t *src,
                          apr_pool_t *pool)
{
#if APR_HAS_XLATE || !WIN32
  xlate_handle_node_t *node;
  svn_error_t *err;

  SVN_ERR(get_ntou_xlate_handle_node(&node, pool));

  if (node->handle)
    {
      err = convert_to_stringbuf(node, src->data, src->len, dest, pool);
      if (! err)
        err = check_utf8((*dest)->data, (*dest)->len, pool);
    }
  else
    {
      err = check_non_ascii(src->data, src->len, pool);
      if (! err)
        *dest = svn_stringbuf_dup(src, pool);
    }

  put_xlate_handle_node(node, SVN_UTF_NTOU_XLATE_HANDLE, pool);

  return err;
#else
  apr_size_t widelen = 0;
  apr_size_t utf8len = 0;
  wchar_t * widebuf = NULL;

  widelen = MultiByteToWideChar(CP_ACP, 0, src->data, src->len, NULL, 0);
  if (widelen == 0)
    return svn_error_createf(APR_EINVAL, NULL, 
                             _("Error converting string to utf8\n"));

  widebuf = calloc(widelen, sizeof(wchar_t));
  if (widebuf == NULL)
    return svn_error_createf(APR_EINVAL, NULL, 
                  _("Error allocating memory during utf8 conversion\n"));
  widelen = MultiByteToWideChar(CP_ACP, 0, src->data, src->len, widebuf,
                               (*dest)->len);
  if (widelen == 0)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL, 
                                _("Error converting string to utf8\n"));
    }
  utf8len = WideCharToMultiByte(CP_UTF8, 0, widebuf, widelen, NULL, 0,
                                NULL, NULL);
  if (utf8len == 0)
    {
       free(widebuf);
       return svn_error_createf(APR_EINVAL, NULL,
                                 _("Error converting string to utf8\n"));
    }
  *dest = svn_stringbuf_ncreate("", utf8len, pool);
  if (*dest == NULL)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL, 
                 _("Error allocating memory during utf8 conversion\n"));
    }
  utf8len = WideCharToMultiByte(CP_UTF8, 0, widebuf, widelen, 
                               (*dest)->data, (*dest)->len, NULL, NULL);
  free(widebuf);
  if (utf8len == 0)
    return svn_error_createf(APR_EINVAL, NULL,
                             _("Error converting string to utf8\n"));
  return SVN_NO_ERROR;
#endif
}


svn_error_t *
svn_utf_string_to_utf8(const svn_string_t **dest,
                       const svn_string_t *src,
                       apr_pool_t *pool)
{
#if APR_HAS_XLATE || !WIN32
  svn_stringbuf_t *destbuf;
  xlate_handle_node_t *node;
  svn_error_t *err;

  SVN_ERR(get_ntou_xlate_handle_node(&node, pool));

  if (node->handle)
    {
      err = convert_to_stringbuf(node, src->data, src->len, &destbuf, pool);
      if (! err)
        err = check_utf8(destbuf->data, destbuf->len, pool);
      if (! err)
        *dest = svn_string_create_from_buf(destbuf, pool);
    }
  else
    {
      err = check_non_ascii(src->data, src->len, pool);
      if (! err)
        *dest = svn_string_dup(src, pool);
    }

  put_xlate_handle_node(node, SVN_UTF_NTOU_XLATE_HANDLE, pool);

  return err;
#else
  apr_size_t widelen = 0;
  apr_size_t utf8len = 0;
  wchar_t * widebuf = NULL;

  widelen = MultiByteToWideChar(CP_ACP, 0, src->data, src->len, NULL, 0);
  if (widelen == 0)
    return svn_error_createf(APR_EINVAL, NULL, 
                             _("Error converting string to utf8\n"));

  widebuf = calloc(widelen, sizeof(wchar_t));
  if (widebuf == NULL)
    return svn_error_createf(APR_EINVAL, NULL,
                  _("Error allocating memory during utf8 conversion\n"));
  widelen = MultiByteToWideChar(CP_ACP, 0, src->data, src->len, widebuf,
                               (*dest)->len);
  if (widelen == 0)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL, 
                                _("Error converting string to utf8\n"));
    }
  utf8len = WideCharToMultiByte(CP_UTF8, 0, widebuf, widelen, NULL, 0,
                                NULL, NULL);
  if (utf8len == 0)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL, 
                                _("Error converting string to utf8\n"));
    }
  *dest = svn_string_ncreate("", utf8len+1, pool);
  if (*dest == NULL)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL, 
                 _("Error allocating memory during utf8 conversion\n"));
    }
  utf8len = WideCharToMultiByte(CP_UTF8, 0, widebuf, widelen,
                                    (*dest)->data, utf8len, NULL, NULL);
  free(widebuf);
  if (utf8len == 0)
    return svn_error_createf(APR_EINVAL, NULL,
                             _("Error converting string to utf8\n"));
  return SVN_NO_ERROR;
#endif
}

#if APR_HAS_XLATE
/* Common implementation for svn_utf_cstring_to_utf8,
   svn_utf_cstring_to_utf8_ex, svn_utf_cstring_from_utf8 and
   svn_utf_cstring_from_utf8_ex. Convert SRC to DEST using NODE->handle as
   the translator and allocating from POOL. */
static svn_error_t *
convert_cstring(const char **dest,
                const char *src,
                xlate_handle_node_t *node,
                apr_pool_t *pool)
{
  if (node->handle)
    {
      svn_stringbuf_t *destbuf;
      SVN_ERR(convert_to_stringbuf(node, src, strlen(src),
                                   &destbuf, pool));
      *dest = destbuf->data;
    }
  else
    {
      apr_size_t len = strlen(src);
      SVN_ERR(check_non_ascii(src, len, pool));
      *dest = apr_pstrmemdup(pool, src, len);
    }
  return SVN_NO_ERROR;
}
#endif /* APR_HAS_XLATE */

svn_error_t *
svn_utf_cstring_to_utf8(const char **dest,
                        const char *src,
                        apr_pool_t *pool)
{
#if APR_HAS_XLATE || !WIN32
  xlate_handle_node_t *node;
  svn_error_t *err;

  SVN_ERR(get_ntou_xlate_handle_node(&node, pool));
  err = convert_cstring(dest, src, node, pool);
  put_xlate_handle_node(node, SVN_UTF_NTOU_XLATE_HANDLE, pool);
  SVN_ERR(err);
  SVN_ERR(check_cstring_utf8(*dest, pool));

  return SVN_NO_ERROR;
#else
  apr_size_t widelen = 0;
  apr_size_t utf8len = 0;
  wchar_t * widebuf = NULL;

  widelen = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
  if (widelen == 0)
    return svn_error_createf(APR_EINVAL, NULL, 
                             _("Error converting string to utf8\n"));

  widebuf = calloc(widelen, sizeof(wchar_t));
  if (widebuf == NULL)
    return svn_error_createf(APR_EINVAL, NULL, 
                  _("Error allocating memory during utf8 conversion\n"));
  widelen = MultiByteToWideChar(CP_ACP, 0, src, -1, widebuf, widelen);
  if (widelen == 0)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL, 
                                _("Error converting string to utf8\n"));
    }
  utf8len = WideCharToMultiByte(CP_UTF8, 0, widebuf, widelen, NULL, 0,
                                NULL, NULL);
  if (utf8len == 0)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL, 
                                _("Error converting string to utf8\n"));
    }
  *dest = apr_pstrmemdup(pool, "", utf8len+1);
  if (*dest == NULL)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL, 
                 _("Error allocating memory during utf8 conversion\n"));
    }
  utf8len = WideCharToMultiByte(CP_UTF8, 0, widebuf, widelen, 
                                *dest, utf8len, NULL, NULL);
  free(widebuf);
  if (utf8len == 0)
    return svn_error_createf(APR_EINVAL, NULL, 
                              _("Error converting string to utf8\n"));
  return SVN_NO_ERROR;
#endif
}


svn_error_t *
svn_utf_cstring_to_utf8_ex2(const char **dest,
                            const char *src,
                            const char *frompage,
                            apr_pool_t *pool)
{
#if APR_HAS_XLATE || !WIN32
  xlate_handle_node_t *node;
  svn_error_t *err;
  const char *convset_key = get_xlate_key(SVN_APR_UTF8_CHARSET, frompage,
                                          pool);

  SVN_ERR(get_xlate_handle_node(&node, SVN_APR_UTF8_CHARSET, frompage,
                                convset_key, pool));
  err = convert_cstring(dest, src, node, pool);
  put_xlate_handle_node(node, convset_key, pool);
  SVN_ERR(err);
  SVN_ERR(check_cstring_utf8(*dest, pool));

  return SVN_NO_ERROR;
#else
  /* UI clients most likely never use the frompage param but leave
     it NULL since they don't change the thread locale they're running
     on. Which means converting from/to the 'current locale' is always
     ok. */
  assert(frompage == NULL);
  return svn_utf_cstring_to_utf8(dest, src, pool);
#endif
}


svn_error_t *
svn_utf_cstring_to_utf8_ex(const char **dest,
                           const char *src,
                           const char *frompage,
                           const char *convset_key,
                           apr_pool_t *pool)
{
  return svn_utf_cstring_to_utf8_ex2(dest, src, frompage, pool);
}


svn_error_t *
svn_utf_stringbuf_from_utf8(svn_stringbuf_t **dest,
                            const svn_stringbuf_t *src,
                            apr_pool_t *pool)
{
#if APR_HAS_XLATE || !WIN32
  xlate_handle_node_t *node;
  svn_error_t *err;

  SVN_ERR(get_uton_xlate_handle_node(&node, pool));

  if (node->handle)
    {
      err = check_utf8(src->data, src->len, pool);
      if (! err)
        err = convert_to_stringbuf(node, src->data, src->len, dest, pool);
    }
  else
    {
      err = check_non_ascii(src->data, src->len, pool);
      if (! err)
        *dest = svn_stringbuf_dup(src, pool);
    }

  put_xlate_handle_node(node, SVN_UTF_UTON_XLATE_HANDLE, pool);

  return err;
#else
  apr_size_t widelen = 0;
  apr_size_t acplen = 0;
  wchar_t * widebuf = NULL;

  widelen = MultiByteToWideChar(CP_UTF8, 0, src->data, src->len, NULL, 0);
  if (widelen == 0)
    return svn_error_createf(APR_EINVAL, NULL, 
                              _("Error converting string to utf8\n"));

  widebuf = calloc(widelen, sizeof(wchar_t));
  if (widebuf == NULL)
    return svn_error_createf(APR_EINVAL, NULL, 
                  _("Error allocating memory during utf8 conversion\n"));
  widelen = MultiByteToWideChar(CP_UTF8, 0, src->data, src->len,
                                widebuf, widelen);
  if (widelen == 0)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL,
                                _("Error converting string to utf8\n"));
    }
  acplen = WideCharToMultiByte(CP_ACP, 0, widebuf, widelen, NULL, 0,
                               NULL, NULL);
  if (acplen == 0)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL,
                                _("Error converting string to utf8\n"));
    }
  *dest = svn_stringbuf_ncreate("", acplen, pool);
  if (*dest == NULL)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL, 
                  _("Error allocating memory during utf8 conversion\n"));
    }
  acplen = WideCharToMultiByte(CP_ACP, 0, widebuf, widelen,
                               (*dest)->data, (*dest)->len, NULL, NULL);
  free(widebuf);
  if (acplen == 0)
    return svn_error_createf(APR_EINVAL, NULL,
                              _("Error converting string to utf8\n"));
  return SVN_NO_ERROR;
#endif
}


svn_error_t *
svn_utf_string_from_utf8(const svn_string_t **dest,
                         const svn_string_t *src,
                         apr_pool_t *pool)
{
#if APR_HAS_XLATE || !WIN32
  svn_stringbuf_t *dbuf;
  xlate_handle_node_t *node;
  svn_error_t *err;

  SVN_ERR(get_uton_xlate_handle_node(&node, pool));

  if (node->handle)
    {
      err = check_utf8(src->data, src->len, pool);
      if (! err)
        err = convert_to_stringbuf(node, src->data, src->len,
                                   &dbuf, pool);
      if (! err)
        *dest = svn_string_create_from_buf(dbuf, pool);
    }
  else
    {
      err = check_non_ascii(src->data, src->len, pool);
      if (! err)
        *dest = svn_string_dup(src, pool);
    }

  put_xlate_handle_node(node, SVN_UTF_UTON_XLATE_HANDLE, pool);

  return err;
#else
  apr_size_t widelen = 0;
  apr_size_t acplen = 0;
  wchar_t * widebuf = NULL;

  widelen = MultiByteToWideChar(CP_UTF8, 0, src->data, src->len, NULL, 0);
  if (widelen == 0)
    return svn_error_createf(APR_EINVAL, NULL,
                              _("Error converting string to utf8\n"));

  widebuf = calloc(widelen, sizeof(wchar_t));
  if (widebuf == NULL)
    return svn_error_createf(APR_EINVAL, NULL,
                  _("Error allocating memory during utf8 conversion\n"));
  widelen = MultiByteToWideChar(CP_UTF8, 0, src->data, src->len,
                                widebuf, widelen);
  if (widelen == 0)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL,
                                _("Error converting string to utf8\n"));
    }
  acplen = WideCharToMultiByte(CP_ACP, 0, widebuf, widelen, NULL, 0,
                               NULL, NULL);
  if (acplen == 0)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL,
                                _("Error converting string to utf8\n"));
    }
  *dest = svn_string_ncreate("", acplen, pool);
  if (*dest == NULL)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL,
                  _("Error allocating memory during utf8 conversion\n"));
    }
  acplen = WideCharToMultiByte(CP_ACP, 0, widebuf, widelen,
                               (*dest)->data, (*dest)->len, NULL, NULL);
  free(widebuf);
  if (acplen == 0)
    return svn_error_createf(APR_EINVAL, NULL,
                              _("Error converting string to utf8\n"));
  return SVN_NO_ERROR;
#endif
}


svn_error_t *
svn_utf_cstring_from_utf8(const char **dest,
                          const char *src,
                          apr_pool_t *pool)
{
#if APR_HAS_XLATE || !WIN32
  xlate_handle_node_t *node;
  svn_error_t *err;

  SVN_ERR(check_utf8(src, strlen(src), pool));

  SVN_ERR(get_uton_xlate_handle_node(&node, pool));
  err = convert_cstring(dest, src, node, pool);
  put_xlate_handle_node(node, SVN_UTF_UTON_XLATE_HANDLE, pool);

  return err;
#else
  apr_size_t widelen = 0;
  apr_size_t acplen = 0;
  wchar_t * widebuf = NULL;

  widelen = MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
  if (widelen == 0)
    return svn_error_createf(APR_EINVAL, NULL,
                              _("Error converting string to utf8\n"));

  widebuf = calloc(widelen, sizeof(wchar_t));
  if (widebuf == NULL)
    return svn_error_createf(APR_EINVAL, NULL,
                  _("Error allocating memory during utf8 conversion\n"));
  widelen = MultiByteToWideChar(CP_UTF8, 0, src, -1, widebuf, widelen);
  if (widelen == 0)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL,
                                _("Error converting string to utf8\n"));
    }
  acplen = WideCharToMultiByte(CP_ACP, 0, widebuf, widelen, NULL, 0, 
                               NULL, NULL);
  if (acplen == 0)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL,
                                _("Error converting string to utf8\n"));
    }
  *dest = apr_pstrmemdup(pool, "", acplen+1);
  if (*dest == NULL)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL,
                 _("Error allocating memory during utf8 conversion\n"));
    }
  acplen = WideCharToMultiByte(CP_ACP, 0, widebuf, widelen,
                               *dest, acplen, NULL, NULL);
  free(widebuf);
  if (acplen == 0)
    return svn_error_createf(APR_EINVAL, NULL,
                              _("Error converting string to utf8\n"));
  return SVN_NO_ERROR;
#endif
}


svn_error_t *
svn_utf_cstring_from_utf8_ex2(const char **dest,
                              const char *src,
                              const char *topage,
                              apr_pool_t *pool)
{
#if APR_HAS_XLATE || !WIN32
  xlate_handle_node_t *node;
  svn_error_t *err;
  const char *convset_key = get_xlate_key(topage, SVN_APR_UTF8_CHARSET,
                                          pool);

  SVN_ERR(check_utf8(src, strlen(src), pool));

  SVN_ERR(get_xlate_handle_node(&node, topage, SVN_APR_UTF8_CHARSET,
                                convset_key, pool));
  err = convert_cstring(dest, src, node, pool);
  put_xlate_handle_node(node, convset_key, pool);

  return err;
#else
  assert(topage == NULL);
  return svn_utf_cstring_from_utf8(dest, src, pool);
#endif
}


svn_error_t *
svn_utf_cstring_from_utf8_ex(const char **dest,
                             const char *src,
                             const char *topage,
                             const char *convset_key,
                             apr_pool_t *pool)
{
  return svn_utf_cstring_from_utf8_ex2(dest, src, topage, pool);
}


const char *
svn_utf__cstring_from_utf8_fuzzy(const char *src,
                                 apr_pool_t *pool,
                                 svn_error_t *(*convert_from_utf8)
                                 (const char **, const char *, apr_pool_t *))
{
  const char *escaped, *converted;
  svn_error_t *err;

  escaped = fuzzy_escape(src, strlen(src), pool);

  /* Okay, now we have a *new* UTF-8 string, one that's guaranteed to
     contain only 7-bit bytes :-).  Recode to native... */
  err = convert_from_utf8(((const char **) &converted), escaped, pool);

  if (err)
    {
      svn_error_clear(err);
      return escaped;
    }
  else
    return converted;

  /* ### Check the client locale, maybe we can avoid that second
   * conversion!  See Ulrich Drepper's patch at
   * http://subversion.tigris.org/issues/show_bug.cgi?id=807.
   */
}


const char *
svn_utf_cstring_from_utf8_fuzzy(const char *src,
                                apr_pool_t *pool)
{
  return svn_utf__cstring_from_utf8_fuzzy(src, pool,
                                          svn_utf_cstring_from_utf8);
}


svn_error_t *
svn_utf_cstring_from_utf8_stringbuf(const char **dest,
                                    const svn_stringbuf_t *src,
                                    apr_pool_t *pool)
{
  svn_stringbuf_t *destbuf;

  SVN_ERR(svn_utf_stringbuf_from_utf8(&destbuf, src, pool));
  *dest = destbuf->data;

  return SVN_NO_ERROR;
}


svn_error_t *
svn_utf_cstring_from_utf8_string(const char **dest,
                                 const svn_string_t *src,
                                 apr_pool_t *pool)
{
#if APR_HAS_XLATE || !WIN32
  svn_stringbuf_t *dbuf;
  xlate_handle_node_t *node;
  svn_error_t *err;

  SVN_ERR(get_uton_xlate_handle_node(&node, pool));

  if (node->handle)
    {
      err = check_utf8(src->data, src->len, pool);
      if (! err)
        err = convert_to_stringbuf(node, src->data, src->len,
                                   &dbuf, pool);
      if (! err)
        *dest = dbuf->data;
    }
  else
    {
      err = check_non_ascii(src->data, src->len, pool);
      if (! err)
        *dest = apr_pstrmemdup(pool, src->data, src->len);
    }

  put_xlate_handle_node(node, SVN_UTF_UTON_XLATE_HANDLE, pool);

  return err;
#else
  apr_size_t widelen = 0;
  apr_size_t acplen = 0;
  wchar_t * widebuf = NULL;

  widelen = MultiByteToWideChar(CP_UTF8, 0, src->data, src->len, NULL, 0);
  if (widelen == 0)
    return svn_error_createf(APR_EINVAL, NULL,
                              _("Error converting string to utf8\n"));

  widebuf = calloc(widelen, sizeof(wchar_t));
  if (widebuf == NULL)
    return svn_error_createf(APR_EINVAL, NULL,
                  _("Error allocating memory during utf8 conversion\n"));
  widelen = MultiByteToWideChar(CP_UTF8, 0, src->data, src->len,
                                widebuf, widelen);
  if (widelen == 0)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL,
                                _("Error converting string to utf8\n"));
    }
  acplen = WideCharToMultiByte(CP_ACP, 0, widebuf, widelen, NULL, 0,
                               NULL, NULL);
  if (acplen == 0)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL,
                                _("Error converting string to utf8\n"));
    }
  *dest = apr_pstrmemdup(pool, "", acplen+1);
  if (*dest == NULL)
    {
      free(widebuf);
      return svn_error_createf(APR_EINVAL, NULL,
                  _("Error allocating memory during utf8 conversion\n"));
    }
  acplen = WideCharToMultiByte(CP_ACP, 0, widebuf, widelen,
                               *dest, acplen, NULL, NULL);
  free(widebuf);
  if (acplen == 0)
    return svn_error_createf(APR_EINVAL, NULL,
                              _("Error converting string to utf8\n"));
  return SVN_NO_ERROR;
#endif
}
