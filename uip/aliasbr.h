/* aliasbr.h -- new aliasing mechanism
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

/* codes returned by alias() */
/* FIXME: Only AK_OK used externally; interface could be narrower. */
#define	AK_OK		0	/* file parsed OK 	 */
#define	AK_NOFILE	1	/* couldn't read file 	 */
#define	AK_ERROR	2	/* error parsing file 	 */
#define	AK_LIMIT	3	/* memory limit exceeded */

struct aka {
    char *ak_name;		/* name to match against             */
    struct adr *ak_addr;	/* list of addresses that it maps to */
    struct aka *ak_next;	/* next aka in list                  */
    bool ak_visible;		/* should be visible in headers      */
};

/*
 * prototypes
 */
char *akvalue(char *);
int akvisible(void) PURE;
char *akresult(struct aka *);
int alias(char *);
char *akerror(int);

extern struct aka *akahead;

/* FIXME: Definition in config/config.c. */
extern char *AliasFile;		/* mh-alias(5)             */
