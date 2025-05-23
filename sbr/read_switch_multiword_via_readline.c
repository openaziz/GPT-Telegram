/* read_switch_multiword_via_readline.c -- get an answer from the user, with readline
 *
 * This code is Copyright (c) 2012, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "read_switch_multiword_via_readline.h"
#include "smatch.h"
#include "brkstring.h"
#include "ambigsw.h"
#include "print_sw.h"
#include "utils.h"

#ifdef READLINE_SUPPORT
#include <readline/readline.h>
#include <readline/history.h>

static struct swit *rl_cmds;

static char *nmh_command_generator(const char *, int);
static char **nmh_completion(const char *, int, int);
static void initialize_readline(void);

static char ansbuf[BUFSIZ];

/*
 * getans, but with readline support
 */

char **
read_switch_multiword_via_readline(char *prompt, struct swit *ansp)
{
    char *ans, **cpp;

    initialize_readline();
    rl_cmds = ansp;

    for (;;) {
	ans = readline(prompt);
	/*
	 * If we get an EOF, return
	 */

	if (ans == NULL)
	    return NULL;

	if (ans[0] == '?' || ans[0] == '\0') {
	    puts("Options are:");
	    print_sw("", ansp, "", stdout);
	    free(ans);
	    continue;
	}
	add_history(ans);
	strncpy(ansbuf, ans, sizeof(ansbuf));
	ansbuf[sizeof(ansbuf) - 1] = '\0';
        free(ans);

	cpp = brkstring(ansbuf, " ", NULL);
	switch (smatch(*cpp, ansp)) {
	    case AMBIGSW:
		ambigsw(*cpp, ansp);
		continue;
	    case UNKWNSW:
		printf(" -%s unknown. Hit <CR> for help.\n", *cpp);
		continue;
	    default:
		return cpp;
	}
    }
}

static void
initialize_readline(void)
{
    rl_readline_name = "Nmh";
    rl_attempted_completion_function = nmh_completion;
}

static char **
nmh_completion(const char *text, int start, int end)
{
    NMH_UNUSED (end);

    if (start == 0)
	return rl_completion_matches(text, nmh_command_generator);

    return NULL;
}

static char *
nmh_command_generator(const char *text, int state)
{
    static int list_index, len;
    char *name, *p;
    char buf[256];

    if (!state) {
	list_index = 0;
	len = strlen(text);
    }

    while ((name = rl_cmds[list_index].sw)) {
	list_index++;
	strncpy(buf, name, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0';
	p = *brkstring(buf, " ", NULL);
	if (strncmp(p, text, len) == 0)
            return mh_xstrdup(p);
    }

    return NULL;
}
#endif /* READLINE_SUPPORT */
