/* Finit daemon log functions
 *
 * Copyright (c) 2008-2017  Joachim Nilsson <troglobit@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"
#include <sched.h>		/* sched_yield() */
#include <stdio.h>
#include <stdarg.h>
#include <lite/lite.h>

#include "finit.h"
#include "log.h"

static int up       = 0;
static int debug    = 0;
static int silent   = SILENT_MODE;	/* Completely silent, including boot */
static int loglevel = LOG_NOTICE;

void log_init(int dbg)
{
	if (dbg)
		debug = 1;

	if (debug)
		loglevel = LOG_DEBUG;
	else
		loglevel = LOG_NOTICE;
}

/* If we enabled terse mode at boot, restore to previous setting at shutdown */
void log_exit(void)
{
	silent = SILENT_MODE;
	if (!silent) {
		sched_yield();
		fputs("\n", stderr);
	}
}

void log_open(void)
{
	int opts = LOG_PID;

	closelog();

	if (debug)
		opts |= LOG_PERROR; /* LOG_CONS | */
	openlog("finit", opts, LOG_DAEMON);
	setlogmask(LOG_UPTO(loglevel));

	up = 1;
}

void log_silent(void)
{
	if (debug)
		silent = 0;
	else
		silent = 1;
}

int log_is_silent(void)
{
	return silent;
}

/* Toggle debug mode */
void log_debug(void)
{
	debug = !debug;

	if (debug) {
		silent   = 0;
		loglevel = LOG_DEBUG;
	} else {
		silent   = SILENT_MODE;
		loglevel = LOG_NOTICE;
	}
	log_open();

	logit(LOG_NOTICE, "Debug mode %s", debug ? "enabled" : "disabled");
}

int log_is_debug(void)
{
	return debug;
}

static void early_logit(int prio, const char *fmt, va_list ap)
{
	FILE *fp;

	if (!debug && prio > loglevel)
		return;

	fp = fopen("/dev/kmsg", "w");
	if (fp) {
		fprintf(fp, "<%d>finit[1]:", LOG_DAEMON | prio);
		vfprintf(fp, fmt, ap);
		fclose(fp);

		if (debug)
			vfprintf(stderr, fmt, ap);
	} else {
		vfprintf(stderr, fmt, ap);
	}
}

/*
 * Log to /dev/kmsg until syslogd has started, then openlog()
 * and continue logging as a regular daemon.
 */
void logit(int prio, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (!up) {
		if (!fexist("/dev/log")) {
			early_logit(prio, fmt, ap);
			goto done;
		}

		log_open();
	}

	vsyslog(prio, fmt, ap);
done:
	va_end(ap);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
