/* utils.h -- various utility routines
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

/* PLURALS gives a pointer to the string "s" when n isn't 1, and to the
 * empty string "" when it is.  Suitable for obtaining the plural `s'
 * used for English nouns.  It treats -1 as plural, as does GNU gettext.
 * Having output vary for plurals is annoying for those writing parsers;
 * better to phrase the output such that no test is needed, e.g.
 * "messages found: 42". */
extern const char plurals[];
#define PLURALS(n) (plurals + ((n) == 1))

void *mh_xmalloc(size_t) MALLOC ALLOC_SIZE(1);
void *mh_xrealloc(void *, size_t) ALLOC_SIZE(2);
void *mh_xcalloc(size_t, size_t) MALLOC ALLOC_SIZE(1, 2);
char *mh_xstrdup(const char *) MALLOC;
void *xmemtostr(void const *src, size_t len) MALLOC;   /* Can't use ALLOC_SIZE. */

/* Set p to point to newly allocated, uninitialised, memory. */
#define NEW(p) ((p) = mh_xmalloc(sizeof *(p)))

/* Set p to point to newly allocated, zeroed, memory. */
#define NEW0(p) ((p) = mh_xcalloc(1, sizeof *(p)))

/* Zero the bytes to which p points. */
#define ZERO(p) memset((p), 0, sizeof *(p))

char *pwd(void);

char *add(const char *, char *) MALLOC;
char *addlist(char *, const char *) MALLOC;

int folder_exists(const char *);
void create_folder(char *, int, void (*)(int));

int num_digits(int) PURE;

/*
 * A vector of char array, used to hold a list of string message numbers
 * or command arguments.
 */
struct msgs_array {
	int max, size;
	char **msgs;
};

void app_msgarg(struct msgs_array *, char *);

/*
 * Same as msgs_array, but for a vector of ints
 */
struct msgnum_array {
	int max, size;
	int *msgnums;
};

void app_msgnum(struct msgnum_array *, int);

char *find_str(const char [], size_t, const char *) PURE;
char *rfind_str(const char [], size_t, const char *) PURE;

char *nmh_strcasestr(const char *, const char *) PURE;

void trunccpy(char *, const char *, size_t);
/* A convenience for the common case of dest being an array. */
#define TRUNCCPY(dest, src) trunccpy(dest, src, sizeof (dest))

void abortcpy(char *, const char *, size_t);
/* A convenience for the common case of dest being an array. */
#define ABORTCPY(dest, src) abortcpy(dest, src, sizeof (dest))

bool has_prefix(const char *, const char *) PURE;
bool has_suffix(const char *, const char *) PURE;
bool has_suffix_c(const char *, int) PURE;
void trim_suffix_c(char *, int);

void to_lower(char *);
void to_upper(char *);

int nmh_init(const char *, bool, bool);
int nmh_version_changed(int);

bool contains8bit(const char *, const char *);
int scan_input(int, bool *);

char *m_str(int);
char *m_strn(int, unsigned int);
