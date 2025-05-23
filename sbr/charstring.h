/* charstring.h -- dynamically-sized char array that can report size
 *               in both characters and bytes
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

/*
 * char array that keeps track of size in both bytes and characters
 * Usage note:
 *    Don't store return value of charstring_buffer() and use later
 *    after intervening push_back's; use charstring_buffer_copy()
 *    instead.
 */

typedef struct charstring *charstring_t;

charstring_t charstring_create(size_t);
charstring_t charstring_copy(const charstring_t) NONNULL(1);
void charstring_free(charstring_t);
/* Append a single-byte character: */
void charstring_push_back(charstring_t, const char) NONNULL(1);
/* Append possibly multi-byte character(s): */
void charstring_push_back_chars(charstring_t, const char [], size_t, size_t) NONNULL(1);
void charstring_append(charstring_t, const charstring_t) NONNULL(2);
void charstring_append_cstring(charstring_t, const char []) NONNULL(2);
void charstring_clear(charstring_t) NONNULL(1);
/* Don't store return value of charstring_buffer() and use later after
   intervening push_back's; use charstring_buffer_copy() instead. */
const char *charstring_buffer(const charstring_t) NONNULL(1);
/* User is responsible for free'ing result of buffer copy. */
char *charstring_buffer_copy(const charstring_t) NONNULL(1);
size_t charstring_bytes(const charstring_t) NONNULL(1) PURE;
size_t charstring_chars(const charstring_t) NONNULL(1) PURE;
/* Length of the last character in the charstring. */
int charstring_last_char_len(const charstring_t) NONNULL(1);
