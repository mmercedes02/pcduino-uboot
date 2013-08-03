/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * Add to readline cmdline-editing by
 * (C) Copyright 2005
 * JinHua Luo, GuangDong Linux Center, <luo.jinhua@gd-linux.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* #define	DEBUG	*/

#include <common.h>
#include <command.h>
#include <fdtdec.h>
#include <hush.h>
#include <malloc.h>
#include <menu.h>
#include <post.h>
#include <version.h>
#include <watchdog.h>
#include <linux/ctype.h>

#include "lvrenyang.h"
#include <buzzer.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
void inline __show_boot_progress (int val) {}
void show_boot_progress (int val) __attribute__((weak, alias("__show_boot_progress")));

#define MAX_DELAY_STOP_STR 32

#define DEBUG_PARSER	0	/* set to 1 to debug */

#define debug_parser(fmt, args...)		\
	debug_cond(DEBUG_PARSER, fmt, ##args)

#ifndef DEBUG_BOOTKEYS
#define DEBUG_BOOTKEYS 0
#endif
#define debug_bootkeys(fmt, args...)		\
	debug_cond(DEBUG_BOOTKEYS, fmt, ##args)

char        console_buffer[CONFIG_SYS_CBSIZE + 1];	/* console I/O buffer	*/

static char * delete_char (char *buffer, char *p, int *colp, int *np, int plen);
static const char erase_seq[] = "\b \b";		/* erase sequence	*/
static const char   tab_seq[] = "        ";		/* used to expand TABs	*/

#define	endtick(seconds) (get_ticks() + (uint64_t)(seconds) * get_tbclk())

#ifndef CONFIG_BOOT_RETRY_MIN
#define CONFIG_BOOT_RETRY_MIN CONFIG_BOOT_RETRY_TIME
#endif

/***************************************************************************
 * Watch for 'delay' seconds for autoboot stop or autoboot delay string.
 * returns: 0 -  no key string, allow autoboot 1 - got key string, abort
 */
#if defined(CONFIG_BOOTDELAY)
static int abortboot_normal(int bootdelay)
{
	int abort = 0;
	unsigned long ts;

	if (bootdelay >= 0)
		printf("Hit any key to stop autoboot: %2d ", bootdelay);

	while ((bootdelay > 0) && (!abort)) {
		--bootdelay;
		/* delay 1000 ms */
		ts = get_timer(0);
		do {
			if (tstc()) {	/* we got a key press	*/
				abort  = 1;	/* don't auto boot	*/
				bootdelay = 0;	/* no more delay	*/
				(void) getc();  /* consume input	*/
				break;
			}
			udelay(10000);
		} while (!abort && get_timer(ts) < 1000);

		printf("\b\b\b%2d ", bootdelay);
	}

	putc('\n');

	return abort;
}

static int abortboot(int bootdelay)
{
	return abortboot_normal(bootdelay);
}
#endif	/* CONFIG_BOOTDELAY */

#ifdef CONFIG_BOOTDELAY
static void process_boot_delay(void)
{
	char *s;
	int bootdelay;

	s = getenv ("bootdelay");
	bootdelay = s ? (int)simple_strtol(s, NULL, 10) : CONFIG_BOOTDELAY;

	debug ("### main_loop entered: bootdelay=%d\n\n", bootdelay);

	s = getenv ("bootcmd");

	debug ("### main_loop: bootcmd=\"%s\"\n", s ? s : "<UNDEFINED>");

	if (bootdelay != -1 && s && !abortboot(bootdelay)) {
		run_command_list(s, -1, 0);
	}

}
#endif /* CONFIG_BOOTDELAY */

void main_loop(void)
{

	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");

#ifdef CONFIG_SYS_HUSH_PARSER
	u_boot_hush_start();
#endif

#ifdef CONFIG_BOOTDELAY
	process_boot_delay();
#endif
	/*
	 * Main Loop for Monitor Command Processing
	 */
	//buzzer_hang(4000);
	/* put my code in here */
	parse_file_outer();
	/* This point is never reached */
	for (;;);
}

#ifdef CONFIG_CMDLINE_EDITING

/*
 * cmdline-editing related codes from vivi.
 * Author: Janghoon Lyu <nandy@mizi.com>
 */

#define putnstr(str,n)	do {			\
		printf ("%.*s", (int)n, str);	\
	} while (0)

#define CTL_CH(c)		((c) - 'a' + 1)
#define CTL_BACKSPACE		('\b')
#define DEL			((char)255)
#define DEL7			((char)127)
#define CREAD_HIST_CHAR		('!')

#define getcmd_putch(ch)	putc(ch)
#define getcmd_getch()		getc()
#define getcmd_cbeep()		getcmd_putch('\a')

#define HIST_MAX		20
#define HIST_SIZE		CONFIG_SYS_CBSIZE

static int hist_max;
static int hist_add_idx;
static int hist_cur = -1;
static unsigned hist_num;

static char *hist_list[HIST_MAX];
static char hist_lines[HIST_MAX][HIST_SIZE + 1];	/* Save room for NULL */

#define add_idx_minus_one() ((hist_add_idx == 0) ? hist_max : hist_add_idx-1)

static void hist_init(void)
{
	int i;

	hist_max = 0;
	hist_add_idx = 0;
	hist_cur = -1;
	hist_num = 0;

	for (i = 0; i < HIST_MAX; i++) {
		hist_list[i] = hist_lines[i];
		hist_list[i][0] = '\0';
	}
}

static void cread_add_to_hist(char *line)
{
	strcpy(hist_list[hist_add_idx], line);

	if (++hist_add_idx >= HIST_MAX)
		hist_add_idx = 0;

	if (hist_add_idx > hist_max)
		hist_max = hist_add_idx;

	hist_num++;
}

static char* hist_prev(void)
{
	char *ret;
	int old_cur;

	if (hist_cur < 0)
		return NULL;

	old_cur = hist_cur;
	if (--hist_cur < 0)
		hist_cur = hist_max;

	if (hist_cur == hist_add_idx) {
		hist_cur = old_cur;
		ret = NULL;
	} else
		ret = hist_list[hist_cur];

	return (ret);
}

static char* hist_next(void)
{
	char *ret;

	if (hist_cur < 0)
		return NULL;

	if (hist_cur == hist_add_idx)
		return NULL;

	if (++hist_cur > hist_max)
		hist_cur = 0;

	if (hist_cur == hist_add_idx) {
		ret = "";
	} else
		ret = hist_list[hist_cur];

	return (ret);
}

#ifndef CONFIG_CMDLINE_EDITING
static void cread_print_hist_list(void)
{
	int i;
	unsigned long n;

	n = hist_num - hist_max;

	i = hist_add_idx + 1;
	while (1) {
		if (i > hist_max)
			i = 0;
		if (i == hist_add_idx)
			break;
		printf("%s\n", hist_list[i]);
		n++;
		i++;
	}
}
#endif /* CONFIG_CMDLINE_EDITING */

#define BEGINNING_OF_LINE() {			\
	while (num) {				\
		getcmd_putch(CTL_BACKSPACE);	\
		num--;				\
	}					\
}

#define ERASE_TO_EOL() {				\
	if (num < eol_num) {				\
		printf("%*s", (int)(eol_num - num), ""); \
		do {					\
			getcmd_putch(CTL_BACKSPACE);	\
		} while (--eol_num > num);		\
	}						\
}

#define REFRESH_TO_EOL() {			\
	if (num < eol_num) {			\
		wlen = eol_num - num;		\
		putnstr(buf + num, wlen);	\
		num = eol_num;			\
	}					\
}

static void cread_add_char(char ichar, int insert, unsigned long *num,
	       unsigned long *eol_num, char *buf, unsigned long len)
{
	unsigned long wlen;

	/* room ??? */
	if (insert || *num == *eol_num) {
		if (*eol_num > len - 1) {
			getcmd_cbeep();
			return;
		}
		(*eol_num)++;
	}

	if (insert) {
		wlen = *eol_num - *num;
		if (wlen > 1) {
			memmove(&buf[*num+1], &buf[*num], wlen-1);
		}

		buf[*num] = ichar;
		putnstr(buf + *num, wlen);
		(*num)++;
		while (--wlen) {
			getcmd_putch(CTL_BACKSPACE);
		}
	} else {
		/* echo the character */
		wlen = 1;
		buf[*num] = ichar;
		putnstr(buf + *num, wlen);
		(*num)++;
	}
}

static void cread_add_str(char *str, int strsize, int insert, unsigned long *num,
	      unsigned long *eol_num, char *buf, unsigned long len)
{
	while (strsize--) {
		cread_add_char(*str, insert, num, eol_num, buf, len);
		str++;
	}
}

static int cread_line(const char *const prompt, char *buf, unsigned int *len,
		int timeout)
{
	unsigned long num = 0;
	unsigned long eol_num = 0;
	unsigned long wlen;
	char ichar;
	int insert = 1;
	int esc_len = 0;
	char esc_save[8];
	int init_len = strlen(buf);
	int first = 1;

	if (init_len)
		cread_add_str(buf, init_len, 1, &num, &eol_num, buf, *len);

	while (1) {
#ifdef CONFIG_BOOT_RETRY_TIME
		while (!tstc()) {	/* while no incoming data */
			if (retry_time >= 0 && get_ticks() > endtime)
				return (-2);	/* timed out */
			WATCHDOG_RESET();
		}
#endif
		if (first && timeout) {
			uint64_t etime = endtick(timeout);

			while (!tstc()) {	/* while no incoming data */
				if (get_ticks() >= etime)
					return -2;	/* timed out */
				WATCHDOG_RESET();
			}
			first = 0;
		}

		ichar = getcmd_getch();

		if ((ichar == '\n') || (ichar == '\r')) {
			putc('\n');
			break;
		}

		/*
		 * handle standard linux xterm esc sequences for arrow key, etc.
		 */
		if (esc_len != 0) {
			if (esc_len == 1) {
				if (ichar == '[') {
					esc_save[esc_len] = ichar;
					esc_len = 2;
				} else {
					cread_add_str(esc_save, esc_len, insert,
						      &num, &eol_num, buf, *len);
					esc_len = 0;
				}
				continue;
			}

			switch (ichar) {

			case 'D':	/* <- key */
				ichar = CTL_CH('b');
				esc_len = 0;
				break;
			case 'C':	/* -> key */
				ichar = CTL_CH('f');
				esc_len = 0;
				break;	/* pass off to ^F handler */
			case 'H':	/* Home key */
				ichar = CTL_CH('a');
				esc_len = 0;
				break;	/* pass off to ^A handler */
			case 'A':	/* up arrow */
				ichar = CTL_CH('p');
				esc_len = 0;
				break;	/* pass off to ^P handler */
			case 'B':	/* down arrow */
				ichar = CTL_CH('n');
				esc_len = 0;
				break;	/* pass off to ^N handler */
			default:
				esc_save[esc_len++] = ichar;
				cread_add_str(esc_save, esc_len, insert,
					      &num, &eol_num, buf, *len);
				esc_len = 0;
				continue;
			}
		}

		switch (ichar) {
		case 0x1b:
			if (esc_len == 0) {
				esc_save[esc_len] = ichar;
				esc_len = 1;
			} else {
				puts("impossible condition #876\n");
				esc_len = 0;
			}
			break;

		case CTL_CH('a'):
			BEGINNING_OF_LINE();
			break;
		case CTL_CH('c'):	/* ^C - break */
			*buf = '\0';	/* discard input */
			return (-1);
		case CTL_CH('f'):
			if (num < eol_num) {
				getcmd_putch(buf[num]);
				num++;
			}
			break;
		case CTL_CH('b'):
			if (num) {
				getcmd_putch(CTL_BACKSPACE);
				num--;
			}
			break;
		case CTL_CH('d'):
			if (num < eol_num) {
				wlen = eol_num - num - 1;
				if (wlen) {
					memmove(&buf[num], &buf[num+1], wlen);
					putnstr(buf + num, wlen);
				}

				getcmd_putch(' ');
				do {
					getcmd_putch(CTL_BACKSPACE);
				} while (wlen--);
				eol_num--;
			}
			break;
		case CTL_CH('k'):
			ERASE_TO_EOL();
			break;
		case CTL_CH('e'):
			REFRESH_TO_EOL();
			break;
		case CTL_CH('o'):
			insert = !insert;
			break;
		case CTL_CH('x'):
		case CTL_CH('u'):
			BEGINNING_OF_LINE();
			ERASE_TO_EOL();
			break;
		case DEL:
		case DEL7:
		case 8:
			if (num) {
				wlen = eol_num - num;
				num--;
				memmove(&buf[num], &buf[num+1], wlen);
				getcmd_putch(CTL_BACKSPACE);
				putnstr(buf + num, wlen);
				getcmd_putch(' ');
				do {
					getcmd_putch(CTL_BACKSPACE);
				} while (wlen--);
				eol_num--;
			}
			break;
		case CTL_CH('p'):
		case CTL_CH('n'):
		{
			char * hline;

			esc_len = 0;

			if (ichar == CTL_CH('p'))
				hline = hist_prev();
			else
				hline = hist_next();

			if (!hline) {
				getcmd_cbeep();
				continue;
			}

			/* nuke the current line */
			/* first, go home */
			BEGINNING_OF_LINE();

			/* erase to end of line */
			ERASE_TO_EOL();

			/* copy new line into place and display */
			strcpy(buf, hline);
			eol_num = strlen(buf);
			REFRESH_TO_EOL();
			continue;
		}
#ifdef CONFIG_AUTO_COMPLETE
		case '\t': {
			int num2, col;

			/* do not autocomplete when in the middle */
			if (num < eol_num) {
				getcmd_cbeep();
				break;
			}

			buf[num] = '\0';
			col = strlen(prompt) + eol_num;
			num2 = num;
			if (cmd_auto_complete(prompt, buf, &num2, &col)) {
				col = num2 - num;
				num += col;
				eol_num += col;
			}
			break;
		}
#endif
		default:
			cread_add_char(ichar, insert, &num, &eol_num, buf, *len);
			break;
		}
	}
	*len = eol_num;
	buf[eol_num] = '\0';	/* lose the newline */

	if (buf[0] && buf[0] != CREAD_HIST_CHAR)
		cread_add_to_hist(buf);
	hist_cur = hist_add_idx;

	return 0;
}

#endif /* CONFIG_CMDLINE_EDITING */

/****************************************************************************/

/*
 * Prompt for input and read a line.
 * If  CONFIG_BOOT_RETRY_TIME is defined and retry_time >= 0,
 * time out when time goes past endtime (timebase time in ticks).
 * Return:	number of read characters
 *		-1 if break
 *		-2 if timed out
 */
int readline (const char *const prompt)
{
	/*
	 * If console_buffer isn't 0-length the user will be prompted to modify
	 * it instead of entering it from scratch as desired.
	 */
	console_buffer[0] = '\0';

	return readline_into_buffer(prompt, console_buffer, 0);
}


int readline_into_buffer(const char *const prompt, char *buffer, int timeout)
{
	char *p = buffer;
#ifdef CONFIG_CMDLINE_EDITING
	unsigned int len = CONFIG_SYS_CBSIZE;
	int rc;
	static int initted = 0;

	/*
	 * History uses a global array which is not
	 * writable until after relocation to RAM.
	 * Revert to non-history version if still
	 * running from flash.
	 */
	if (gd->flags & GD_FLG_RELOC) {
		if (!initted) {
			hist_init();
			initted = 1;
		}

		if (prompt)
			puts (prompt);

		rc = cread_line(prompt, p, &len, timeout);
		return rc < 0 ? rc : len;

	} else {
#endif	/* CONFIG_CMDLINE_EDITING */
	char * p_buf = p;
	int	n = 0;				/* buffer index		*/
	int	plen = 0;			/* prompt length	*/
	int	col;				/* output column cnt	*/
	char	c;

	/* print prompt */
	if (prompt) {
		plen = strlen (prompt);
		puts (prompt);
	}
	col = plen;

	for (;;) {
#ifdef CONFIG_BOOT_RETRY_TIME
		while (!tstc()) {	/* while no incoming data */
			if (retry_time >= 0 && get_ticks() > endtime)
				return (-2);	/* timed out */
			WATCHDOG_RESET();
		}
#endif
		WATCHDOG_RESET();		/* Trigger watchdog, if needed */

#ifdef CONFIG_SHOW_ACTIVITY
		while (!tstc()) {
			show_activity(0);
			WATCHDOG_RESET();
		}
#endif
		c = getc();

		/*
		 * Special character handling
		 */
		switch (c) {
		case '\r':			/* Enter		*/
		case '\n':
			*p = '\0';
			puts ("\r\n");
			return p - p_buf;

		case '\0':			/* nul			*/
			continue;

		case 0x03:			/* ^C - break		*/
			p_buf[0] = '\0';	/* discard input */
			return -1;

		case 0x15:			/* ^U - erase line	*/
			while (col > plen) {
				puts (erase_seq);
				--col;
			}
			p = p_buf;
			n = 0;
			continue;

		case 0x17:			/* ^W - erase word	*/
			p=delete_char(p_buf, p, &col, &n, plen);
			while ((n > 0) && (*p != ' ')) {
				p=delete_char(p_buf, p, &col, &n, plen);
			}
			continue;

		case 0x08:			/* ^H  - backspace	*/
		case 0x7F:			/* DEL - backspace	*/
			p=delete_char(p_buf, p, &col, &n, plen);
			continue;

		default:
			/*
			 * Must be a normal character then
			 */
			if (n < CONFIG_SYS_CBSIZE-2) {
				if (c == '\t') {	/* expand TABs */
#ifdef CONFIG_AUTO_COMPLETE
					/* if auto completion triggered just continue */
					*p = '\0';
					if (cmd_auto_complete(prompt, console_buffer, &n, &col)) {
						p = p_buf + n;	/* reset */
						continue;
					}
#endif
					puts (tab_seq+(col&07));
					col += 8 - (col&07);
				} else {
					char buf[2];

					/*
					 * Echo input using puts() to force an
					 * LCD flush if we are using an LCD
					 */
					++col;
					buf[0] = c;
					buf[1] = '\0';
					puts(buf);
				}
				*p++ = c;
				++n;
			} else {			/* Buffer full		*/
				putc ('\a');
			}
		}
	}
#ifdef CONFIG_CMDLINE_EDITING
	}
#endif
}

/****************************************************************************/

static char * delete_char (char *buffer, char *p, int *colp, int *np, int plen)
{
	char *s;

	if (*np == 0) {
		return (p);
	}

	if (*(--p) == '\t') {			/* will retype the whole line	*/
		while (*colp > plen) {
			puts (erase_seq);
			(*colp)--;
		}
		for (s=buffer; s<p; ++s) {
			if (*s == '\t') {
				puts (tab_seq+((*colp) & 07));
				*colp += 8 - ((*colp) & 07);
			} else {
				++(*colp);
				putc (*s);
			}
		}
	} else {
		puts (erase_seq);
		(*colp)--;
	}
	(*np)--;
	return (p);
}

/****************************************************************************/

int parse_line (char *line, char *argv[])
{
	int nargs = 0;

	debug_parser("parse_line: \"%s\"\n", line);
	while (nargs < CONFIG_SYS_MAXARGS) {

		/* skip any white space */
		while (isblank(*line))
			++line;

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			debug_parser("parse_line: nargs=%d\n", nargs);
			return nargs;
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && !isblank(*line))
			++line;

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			debug_parser("parse_line: nargs=%d\n", nargs);
			return nargs;
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

	printf ("** Too many args (max. %d) **\n", CONFIG_SYS_MAXARGS);

	debug_parser("parse_line: nargs=%d\n", nargs);
	return (nargs);
}

/****************************************************************************/

/*
 * Run a command using the selected parser.
 *
 * @param cmd	Command to run
 * @param flag	Execution flags (CMD_FLAG_...)
 * @return 0 on success, or != 0 on error.
 */
int run_command(const char *cmd, int flag)
{
	return parse_string_outer(cmd,
			FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP);
}

int run_command_list(const char *cmd, int len, int flag)
{
	int need_buff = 1;
	char *buff = (char *)cmd;	/* cast away const */
	int rcode = 0;

	if (len == -1) {
		len = strlen(cmd);
		
		/* hush will never change our string */
		need_buff = 0;
	}
	if (need_buff) {
		buff = malloc(len + 1);
		if (!buff)
			return 1;
		memcpy(buff, cmd, len);
		buff[len] = '\0';
	}

	rcode = parse_string_outer(buff, FLAG_PARSE_SEMICOLON);

	return rcode;
}

/****************************************************************************/

#if defined(CONFIG_CMD_RUN)
int do_run (cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	int i;

	if (argc < 2)
		return CMD_RET_USAGE;

	for (i=1; i<argc; ++i) {
		char *arg;

		if ((arg = getenv (argv[i])) == NULL) {
			printf ("## Error: \"%s\" not defined\n", argv[i]);
			return 1;
		}

		if (run_command(arg, flag) != 0)
			return 1;
	}
	return 0;
}
#endif
