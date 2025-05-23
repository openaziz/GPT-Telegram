/* mh.h -- main header file for all of nmh
 */

#include "nmh.h"

/* It's undefined behaviour in C99 to convert from a function pointer to
 * a data-object pointer, e.g. void pointer.  gcc's -pedantic warns of
 * this and can stop compilation.  POSIX requires the operation however,
 * e.g. for dlsym(3), and so we know it's safe on POSIX platforms, e.g.
 * the pointers are of the same size.  Thus use a union to subvert gcc's
 * check.  The function-pointer equivalent of a void pointer is any
 * function-pointer type as all function pointers are defined to be
 * convertible from one another;  use the simplest available. */
typedef union {
    void *v;
    void (*f)(void);
} generic_pointer;

/*
 * Well-used constants
 */
#define	NOTOK        (-1)	/* syscall()s return this on error */
#define	OK             0	/*  ditto on success               */
#define	DONE           1	/* ternary logic                   */

#define MAXARGS	    1000	/* max arguments to exec                */
#define NFOLDERS    1000	/* max folder arguments on command line */
#define DMAXFOLDER     4	/* typical number of digits             */
#define MAXFOLDER   1000	/* message increment                    */

/*
 * This macro is for use by scan, for example, so that platforms with
 * a small BUFSIZ can easily allocate larger buffers.
 */
#define NMH_BUFSIZ  max(BUFSIZ, 8192)

/* If we're using gcc then tell it extra information so it can do more
 * compile-time checks. */
#if __GNUC__ > 2
#define NORETURN __attribute__((__noreturn__))
#define CONST __attribute__((const))
#define MALLOC __attribute__((malloc))
#define NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#define PURE __attribute__((pure))
#define ENDNULL __attribute__((sentinel))
#else
#define NORETURN
#define CHECK_PRINTF(fmt, arg)
#define ALLOC_SIZE(...)
#define CONST
#define MALLOC
#define NONNULL(...)
#define PURE
#define ENDNULL
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define ALLOC_SIZE(...) __attribute__((alloc_size(__VA_ARGS__)))
#define CHECK_PRINTF(fmt, arg) __attribute__((format(printf, fmt, arg)))
#else
#define ALLOC_SIZE(...)
#define CHECK_PRINTF(fmt, arg)
#endif

/* Silence the compiler's "unused variable" warning. */
#define NMH_UNUSED(i) (void)i

/* DIM gives the number of elements in the one-dimensional array a. */
#define DIM(a) (sizeof (a) / sizeof (*(a)))

/* LEN gives the strlen() of string constant s, excluding the
 * terminating NUL. */
#define LEN(s) (sizeof (s) - 1)

/* FENDNULL fends off NULL by giving an empty string instead. */
#define FENDNULL(s) ((s) ? (s) : "")

/* If not specified in a file and PAGER is NULL or empty. */
#define DEFAULT_PAGER "more"

/*
 * user context/profile structure
 */
struct node {
    char *n_name;		/* key                  */
    char *n_field;		/* value                */
    char  n_context;		/* context, not profile */
    struct node *n_next;	/* next entry           */
};

/*
 * switches structure
 */
#define	AMBIGSW	 (-2)	/* from smatch() on ambiguous switch */
#define	UNKWNSW	 (-1)	/* from smatch() on unknown switch   */

struct swit {

    /*
     * Switch name
     */

    char *sw;

    /*
     * The previous comments here about minchars was incorrect; this is
     * (AFAIK) the correct information.
     *
     * A minchars of "0" means this switch can be abbreviated to any number
     * of characters (assuming the abbreviation does not match any other
     * switches).
     *
     * A positive value for minchars means that when the user specifies
     * the switch on the command line, it MUST be at least that many
     * characters.
     *
     * A negative value for minchars means that the user-given switch must
     * be that many characters, but will NOT be shown in -help output.
     *
     * So what should I use?  Well, for nearly all switches you want to specify
     * a minchars of 0.  smatch will report an error if the switch given
     * matches more than one entry.  Let's say you have the following
     * two switches: -append and -apply.  -app will return AMBIGSW from
     * smatch. -appe and -appl will work fine.  So 0 is the correct choice
     * here.
     *
     * The only time you want to specify a minimum length is if you have
     * a switch who's name is a substring of a longer switch.  The example
     * you see sometimes in the code is -form and -format.  If you gave a
     * minchars of 0 for both, -form would match both -form AND -format,
     * and you'd always get AMBIGSW.  The solution is to specify a minchars
     * of 5 for -format; that way just -form will just match -form.  When
     * a minchars is given, the -help output will specify the minimum
     * switch length, like this:
     *
     * -(forma)t string
     *
     * A negative value works the same way, except the switch isn't printed
     * in -help.  Why would you do that?  Well, there are a few instances
     * of internal switches and some switches which only appear if a particular
     * feature is enabled (such as SASL or TLS).  Lately I've been of the
     * opinion that all switches should be specified, even if they are
     * internal or use non-available features, but currently the smatch
     * code still supports this.
     *
     * This isn't the appropriate place to make this note, but since I was
     * here ... when creating switches, you should make a negation switch
     * right after the enabling switch.  E.g. you should have:
     *
     * X("sasl", 0, SASLSW) \
     * X("nosasl", 0, NOSASLSW) \
     *
     * in the switch array, because when you run -help, print_sw will detect
     * this and output:
     *
     * -[no]sasl
     */

    int minchars;

    /*
     * If we pick this switch, return this value from smatch
     */

    int swret;
};

/*
 * Macros to use when declaring struct swit arrays.
 *
 * These macros use a technique known as X-Macros.  In your source code you
 * use them like this:
 *
 * #define FOO_SWITCHES \
 *    X("switch1", 0, SWITCHSW) \
 *    X("switch2", 0, SWITCH2SW) \
 *    X("thirdswitch", 2, SWITCH3SW) \
 *
 * The argument to each entry in FOO_SWITCHES are the switch name (sw),
 * the minchars field (see above) and the return value for this switch.
 * Note that the last entry in the above definition must either omit the
 * continuation backslash, or be followed by a blank line.  In the nmh
 * code the style is to have every line include a backslash and follow
 * the SWITCHES macro definition by a blank line.
 *
 * After you define FOO_SWITCHES, you instantiate it as follows:
 *
 * #define X(sw, minchars, id) id,
 * DEFINE_SWITCH_ENUM(FOO);
 * #undef X
 *
 * #define X(sw, minchars, id) { sw, minchars, id },
 * DEFINE_SWITCH_ARRAY(FOO);
 * #undef X
 *
 * DEFINE_SWITCH_ENUM defines an extra enum at the end of the list called
 * LEN_FOO.
 */

#define DEFINE_SWITCH_ENUM(name) \
    enum { \
	name ## _SWITCHES \
	LEN_ ## name \
    }

#define DEFINE_SWITCH_ARRAY(name, array) \
    static struct swit array[] = { \
	name ## _SWITCHES \
	{ NULL, 0, 0 } \
    }

/*
 * general folder attributes
 */
#define READONLY   (1<<0)	/* No write access to folder    */
#define	SEQMOD	   (1<<1)	/* folder's sequences modified   */
#define	ALLOW_NEW  (1<<2)	/* allow the "new" sequence     */
#define	OTHERS	   (1<<3)	/* folder has other files	*/

#define	FBITS "\020\01READONLY\02SEQMOD\03ALLOW_NEW\04OTHERS"

/*
 * first free slot for user defined sequences
 * and attributes
 */
#define	FFATTRSLOT  4

/*
 * internal messages attributes (sequences)
 */
#define EXISTS        (0)	/* exists            */
#define SELECTED      (1)	/* selected for use  */
#define SELECT_EMPTY  (2)	/* "new" message     */
#define	SELECT_UNSEEN (3)	/* inc/show "unseen" */

#define	MBITS "\020\01EXISTS\02SELECTED\03NEW\04UNSEEN"

#include "sbr/vector.h"

/*
 * Primary structure of folder/message information
 */
struct msgs {
    int lowmsg;		/* Lowest msg number                 */
    int hghmsg;		/* Highest msg number                */
    int nummsg;		/* Actual Number of msgs             */

    int lowsel;		/* Lowest selected msg number        */
    int hghsel;		/* Highest selected msg number       */
    int numsel;		/* Number of msgs selected           */

    int curmsg;		/* Number of current msg if any      */

    int msgflags;	/* Folder attributes (READONLY, etc) */
    char *foldpath;	/* Pathname of folder                */

    /*
     * Name of sequences in this folder.
     */
    svector_t msgattrs;

    /*
     * bit flags for whether sequence
     * is public (0), or private (1)
     */
    bvector_t attrstats;

    /*
     * These represent the lowest and highest possible
     * message numbers we can put in the message status
     * area, without calling folder_realloc().
     */
    int	lowoff;
    int	hghoff;

    /*
     * This is an array of bvector_t which we allocate dynamically.
     * Each bvector_t is a set of bits flags for a particular message.
     * These bit flags represent general attributes such as
     * EXISTS, SELECTED, etc. as well as track if message is
     * in a particular sequence.
     */
    size_t num_msgstats;
    struct bvector *msgstats;	/* msg status */

    /*
     * A FILE handle containing an open filehandle for the sequence file
     * for this folder.  If non-NULL, use it when the sequence file is
     * written.
     */
    FILE *seqhandle;

    /*
     * The name of the public sequence file; required by lkfclose()
     */
    char *seqname;
};

/*
 * Amount of space to allocate for msgstats.  Allocate
 * the array to have space for messages numbered lo to hi.
 * Use MSGSTATNUM to load mp->num_msgstats first.
 */
#define MSGSTATNUM(lo, hi) ((size_t) ((hi) - (lo) + 1))
#define MSGSTATSIZE(mp) ((mp)->num_msgstats * sizeof *(mp)->msgstats)

/*
 * macros for message and sequence manipulation
 */
#define msgstat(mp,n) ((mp)->msgstats + (n) - mp->lowoff)
#define clear_msg_flags(mp,msgnum)   bvector_clear_all (msgstat(mp, msgnum))
#define copy_msg_flags(mp,i,j)       bvector_copy (msgstat(mp,i), msgstat(mp,j))
#define get_msg_flags(mp,ptr,msgnum) bvector_copy (ptr, msgstat(mp, msgnum))
#define set_msg_flags(mp,ptr,msgnum) bvector_copy (msgstat(mp, msgnum), ptr)

#define does_exist(mp,msgnum)     bvector_at (msgstat(mp, msgnum), EXISTS)
#define unset_exists(mp,msgnum)   bvector_clear (msgstat(mp, msgnum), EXISTS)
#define set_exists(mp,msgnum)     bvector_set (msgstat(mp, msgnum), EXISTS)

#define is_selected(mp,msgnum)    bvector_at (msgstat(mp, msgnum), SELECTED)
#define unset_selected(mp,msgnum) bvector_clear (msgstat(mp, msgnum), SELECTED)
#define set_selected(mp,msgnum)   bvector_set (msgstat(mp, msgnum), SELECTED)

#define is_select_empty(mp,msgnum)  \
        bvector_at (msgstat(mp, msgnum), SELECT_EMPTY)
#define set_select_empty(mp,msgnum) \
        bvector_set (msgstat(mp, msgnum), SELECT_EMPTY)

#define is_unseen(mp,msgnum) \
        bvector_at (msgstat(mp, msgnum), SELECT_UNSEEN)
#define unset_unseen(mp,msgnum) \
        bvector_clear (msgstat(mp, msgnum), SELECT_UNSEEN)
#define set_unseen(mp,msgnum) \
        bvector_set (msgstat(mp, msgnum), SELECT_UNSEEN)

#define in_sequence(mp,seqnum,msgnum) \
        bvector_at (msgstat(mp, msgnum), FFATTRSLOT + seqnum)
#define clear_sequence(mp,seqnum,msgnum) \
        bvector_clear (msgstat(mp, msgnum), FFATTRSLOT + seqnum)
#define add_sequence(mp,seqnum,msgnum) \
        bvector_set (msgstat(mp, msgnum), FFATTRSLOT + seqnum)

#define is_seq_private(mp,seqnum) \
        bvector_at (mp->attrstats, FFATTRSLOT + seqnum)
#define make_seq_public(mp,seqnum) \
        bvector_clear (mp->attrstats, FFATTRSLOT + seqnum)
#define make_seq_private(mp,seqnum) \
        bvector_set (mp->attrstats, FFATTRSLOT + seqnum)
#define make_all_public(mp) \
        mp->attrstats = bvector_create(); bvector_clear_all (mp->attrstats)

/*
 * macros for folder attributes
 */
#define clear_folder_flags(mp) ((mp)->msgflags = 0)

#define is_readonly(mp)     ((mp)->msgflags & READONLY)
#define set_readonly(mp)    ((mp)->msgflags |= READONLY)

#define other_files(mp)     ((mp)->msgflags & OTHERS)
#define set_other_files(mp) ((mp)->msgflags |= OTHERS)

/*
 * m_getfld() message parsing
 */

#define NAMESZ  999		/* Limit on component name size.
				   RFC 2822 limits line lengths to
				   998 characters, so a header name
				   can be at most that long.
				   m_getfld limits header names to 2
				   less than NAMESZ, which is fine,
				   because header names must be
				   followed by a colon.	 Add one for
				   terminating NULL. */

/* Token type or error returned from m_getfld(), and its internal state
 * for the next call. */
/* FLD detects the header's name is too long to fit in the fixed size
 * array. */
#define LENERR  (-2)
/* FLD reaches EOF after the header's name, or the name is followed by
 * a linefeed rather than a colon and the body buffer isn't large enough
 * to pretend this header line starts the body. */
#define FMTERR  (-3)
/* The initial state, looking for headers.  Returned when the header's
 * value finishes. */
#define FLD      0
/* Another chunk of the header's value has been returned, but there's
 * more to come. */
#define FLDPLUS  1
/* A chunk of the email's body has been returned. */
#define BODY     3
/* Either the end of the input file has been reached, or the delimiter
 * between emails has been found and the caller should
 * m_getfld_state_reset() to reset the state to FLD for continuing
 * through the file. */
#define FILEEOF  5

typedef struct m_getfld_state *m_getfld_state_t;

#define	NOUSE	0		/* draft being re-used */

#define TFOLDER 0		/* path() given a +folder */
#define TFILE   1		/* path() given a file    */
#define	TSUBCWF	2		/* path() given a @folder */

#define OUTPUTLINELEN	72	/* default line length for headers */

#define LINK	"@"		/* Name of link to file to which you are */
				/* replying. */

/*
 * credentials management
 */
typedef struct nmh_creds *nmh_creds_t;

/*
 * miscellaneous macros
 */
#define	pidXwait(pid,cp) pidstatus (pidwait (pid, NOTOK), stdout, cp)

#ifndef max
# define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
# define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef abs
# define abs(a) ((a) > 0 ? (a) : -(a))
#endif
