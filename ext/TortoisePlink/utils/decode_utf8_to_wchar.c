/*
 * Decode a single UTF-8 character to the platform's local wchar_t.
 */

#include "putty.h"
#include "misc.h"

size_t decode_utf8_to_wchar(const char **utf8, wchar_t *out)
{
    size_t outlen = 0;
    unsigned wc = decode_utf8(utf8);
    if (sizeof(wchar_t) > 2 || wc < 0x10000) {
        out[outlen++] = wc;
    } else {
        unsigned wcoff = wc - 0x10000;
        out[outlen++] = 0xD800 | (0x3FF & (wcoff >> 10));
        out[outlen++] = 0xDC00 | (0x3FF & wcoff);
    }
    return outlen;
}
