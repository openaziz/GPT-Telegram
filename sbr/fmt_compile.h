/* fmt_compile.h -- "compile" format strings for fmt_scan
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

/*
 * This structure describes an "interesting" component.  It holds
 * the name & text from the component (if found) and one piece of
 * auxiliary info.  The structure for a particular component is located
 * by (open) hashing the name and using it as an index into the ptr array
 * "wantcomp".  All format entries that reference a particular component
 * point to its comp struct (so we only have to do component specific
 * processing once.  e.g., parse an address.).
 *
 * In previous implementations "wantcomp" was made available to other
 * functions, but now it's private and is accessed via functions.
 */
struct comp {
    char        *c_name;	/* component name (in lower case) */
    char        *c_text;	/* component text (if found)      */
    struct comp *c_next;	/* hash chain linkage             */
    short        c_flags;	/* misc. flags (from fmt_scan)    */
    short        c_type;	/* type info   (from fmt_compile) */
    union {
	struct tws *c_u_tws;
	struct mailname *c_u_mn;
    } c_un;
    int          c_refcount;	/* Reference count                */
};

#define c_tws c_un.c_u_tws
#define c_mn  c_un.c_u_mn

/*
 * c_flags bits
 */
#define	CF_TRUE       (1<<0)	/* usually means component is present */
#define	CF_PARSED     (1<<1)	/* address/date has been parsed */
#define	CF_DATEFAB    (1<<2)	/* datefield fabricated */
#define CF_TRIMMED    (1<<3)	/* Component has been trimmed */

#define CF_BITS       "\020\01TRUE\02PARSED\03CF_DATEFAB\04TRIMMED"

/*
 * c_type bits
 */
#define	CT_ADDR       (1<<0)	/* referenced as address    */
#define	CT_DATE       (1<<1)	/* referenced as date       */

#define CT_BITS       "\020\01ADDR\02DATE"

/*
 * This structure defines one formatting instruction.
 */
struct format {
    unsigned char f_type;
    char          f_fill;
    short         f_width;	/* output field width   */
    union {
	struct comp *f_u_comp;	/* associated component */
	char        *f_u_text;	/* literal text         */
	char         f_u_char;	/* literal character    */
	int          f_u_value;	/* literal value        */
    } f_un;
    short         f_flags;	/* misc. flags          */
};

#define f_skip f_width		/* instr to skip (false "if") */

#define f_comp  f_un.f_u_comp
#define f_text  f_un.f_u_text
#define f_char  f_un.f_u_char
#define f_value f_un.f_u_value

/* types that output text */
#define FT_COMP          1       /* the text of a component                 */
#define FT_COMPF         2       /* comp text, filled                       */
#define FT_LIT           3       /* literal text                            */
                                 /* FT_LITF removed because it was unused   */
#define FT_CHAR          5       /* a single ASCII character                */
#define FT_NUM           6       /* "value" as decimal number               */
#define FT_NUMF          7       /* "value" as filled dec number            */
#define FT_STR           8       /* "str" as text                           */
#define FT_STRF          9       /* "str" as text, filled                   */
#define FT_STRFW         10      /* "str" as text, filled, width in "value" */
#define FT_STRLIT        11      /* "str" as text, no space compression     */
#define FT_STRLITZ       12      /* literal text with zero display width    */
#define FT_PUTADDR       13      /* split and print address line            */

/* types that modify the "str" or "value" registers                     */
#define FT_LS_COMP       14      /* set "str" to component text          */
#define FT_LS_LIT        15      /* set "str" to literal text            */
#define FT_LS_GETENV     16      /* set "str" to getenv(text)            */
#define FT_LS_CFIND      17      /* set "str" to context_find(text)      */
#define FT_LS_DECODECOMP 18      /* set "str" to decoded component text  */
#define FT_LS_DECODE     19      /* decode "str" as RFC-2047 header      */
#define FT_LS_TRIM       20      /* trim trailing white space from "str" */
#define FT_LV_COMP       21      /* set "value" to comp (as dec. num)    */
#define FT_LV_COMPFLAG   22      /* set "value" to comp flag word        */
#define FT_LV_LIT        23      /* set "value" to literal num           */
#define FT_LV_DAT        24      /* set "value" to dat[n]                */
#define FT_LV_STRLEN     25      /* set "value" to length of "str"       */
#define FT_LV_PLUS_L     26      /* set "value" += literal               */
#define FT_LV_MINUS_L    27      /* set "value" -= literal               */
#define FT_LV_MULTIPLY_L 28      /* set "value" to value * literal       */
#define FT_LV_DIVIDE_L   29      /* set "value" to value / literal       */
#define FT_LV_MODULO_L   30      /* set "value" to value % literal       */
#define FT_LV_CHAR_LEFT  31      /* set "value" to char left in output   */

#define FT_LS_MONTH      32      /* set "str" to tws month                   */
#define FT_LS_LMONTH     33      /* set "str" to long tws month              */
#define FT_LS_ZONE       34      /* set "str" to tws timezone                */
#define FT_LS_DAY        35      /* set "str" to tws weekday                 */
#define FT_LS_WEEKDAY    36      /* set "str" to long tws weekday            */
#define FT_LS_822DATE    37      /* set "str" to 822 date str                */
#define FT_LS_PRETTY     38      /* set "str" to pretty (?) date str         */
#define FT_LS_KILO       39      /* set "str" to "<value>[KMGT]"             */
#define FT_LS_KIBI       40      /* set "str" to "<value>[KMGT]"             */
#define FT_LS_ORDINAL    41      /* set "str" to ordinal suffix of value     */
#define FT_LV_SEC        42      /* set "value" to tws second                */
#define FT_LV_MIN        43      /* set "value" to tws minute                */
#define FT_LV_HOUR       44      /* set "value" to tws hour                  */
#define FT_LV_MDAY       45      /* set "value" to tws day of month          */
#define FT_LV_MON        46      /* set "value" to tws month                 */
#define FT_LV_YEAR       47      /* set "value" to tws year                  */
#define FT_LV_YDAY       48      /* set "value" to tws day of year           */
#define FT_LV_WDAY       49      /* set "value" to tws weekday               */
#define FT_LV_ZONE       50      /* set "value" to tws timezone              */
#define FT_LV_CLOCK      51      /* set "value" to tws clock                 */
#define FT_LV_RCLOCK     52      /* set "value" to now - tws clock           */
#define FT_LV_DAYF       53      /* set "value" to tws day flag              */
#define FT_LV_DST        54      /* set "value" to tws daylight savings flag */
#define FT_LV_ZONEF      55      /* set "value" to tws timezone flag         */

#define FT_LS_PERS       56      /* set "str" to person part of addr    */
#define FT_LS_MBOX       57      /* set "str" to mbox part of addr      */
#define FT_LS_HOST       58      /* set "str" to host part of addr      */
#define FT_LS_PATH       59      /* set "str" to route part of addr     */
#define FT_LS_GNAME      60      /* set "str" to group part of addr     */
#define FT_LS_NOTE       61      /* set "str" to comment part of addr   */
#define FT_LS_ADDR       62      /* set "str" to mbox@host              */
#define FT_LS_822ADDR    63      /* set "str" to 822 format addr        */
#define FT_LS_FRIENDLY   64      /* set "str" to "friendly" format addr */
#define FT_LV_HOSTTYPE   65      /* set "value" to addr host type       */
#define FT_LV_INGRPF     66      /* set "value" to addr in-group flag   */
#define FT_LS_UNQUOTE    67      /* remove RFC 2822 quotes from "str"   */
#define FT_LV_NOHOSTF    68      /* set "value" to addr no-host flag */

/* Date Coercion */
#define FT_LOCALDATE     69      /* Coerce date to local timezone */
#define FT_GMTDATE       70      /* Coerce date to GMT            */

/* pre-format processing */
#define FT_PARSEDATE     71      /* parse comp into a date (tws) struct */
#define FT_PARSEADDR     72      /* parse comp into a mailaddr struct   */
#define FT_FORMATADDR    73      /* let external routine format addr    */
#define FT_CONCATADDR    74      /* formataddr w/out duplicate removal  */
#define FT_MYMBOX        75      /* do "mymbox" test on comp            */
#define FT_GETMYMBOX     76      /* return "mymbox" mailbox string      */
#define FT_GETMYADDR     77      /* return "mymbox" addr string         */

/* conditionals & control flow (must be last) */
#define FT_SAVESTR       78      /* save current str reg               */
#define FT_DONE          79      /* stop formatting                    */
#define FT_PAUSE         80      /* pause                              */
#define FT_NOP           81      /* no-op                              */
#define FT_GOTO          82      /* (relative) goto                    */
#define FT_IF_S_NULL     83      /* test if "str" null                 */
#define FT_IF_S          84      /* test if "str" non-null             */
#define FT_IF_V_EQ       85      /* test if "value" = literal          */
#define FT_IF_V_NE       86      /* test if "value" != literal         */
#define FT_IF_V_GT       87      /* test if "value" > literal          */
#define FT_IF_MATCH      88      /* test if "str" contains literal     */
#define FT_IF_AMATCH     89      /* test if "str" starts with literal  */
#define FT_S_NULL        90      /* V = 1 if "str" null                */
#define FT_S_NONNULL     91      /* V = 1 if "str" non-null            */
#define FT_V_EQ          92      /* V = 1 if "value" = literal         */
#define FT_V_NE          93      /* V = 1 if "value" != literal        */
#define FT_V_GT          94      /* V = 1 if "value" > literal         */
#define FT_V_MATCH       95      /* V = 1 if "str" contains literal    */
#define FT_V_AMATCH      96      /* V = 1 if "str" starts with literal */

#define IF_FUNCS FT_S_NULL       /* start of "if" functions */

/*
 * f_flags bits
 */

#define FF_STRALLOC	(1<<0)	/* String has been allocated */
#define FF_COMPREF	(1<<1)	/* Component reference */

int fmt_compile(char *, struct format **, int);
void fmt_free(struct format *, int);
void fmt_freecomptext(void);
struct comp *fmt_findcomp(char *) PURE;
struct comp *fmt_findcasecomp(char *) PURE;
int fmt_addcompentry(char *);
int fmt_addcomptext(char *, char *);
void fmt_appendcomp(int, char *, char *);
struct comp *fmt_nextcomp(struct comp *, unsigned int *);
