#include "log.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static FILE *logfile;

static void
log_msg (enum log_level level, const char *fmt, va_list args)
{
    struct timespec ts;
    timespec_get (&ts, TIME_UTC);

    char time_buf[100];
    strftime (time_buf, sizeof time_buf, "[%D %T]", gmtime (&ts.tv_sec));

    const char *level_str = log_level_strings[level];

    fprintf (logfile, "%s [%s] ", time_buf, level_str);
    vfprintf (logfile, fmt, args);
}

void
log_error (const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    log_msg (LOG_ERROR, fmt, args);
    va_end (args);
    fprintf (logfile, "\n");
}

void
log_info (const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    log_msg (LOG_INFO, fmt, args);
    va_end (args);
    fprintf (logfile, "\n");
}

void
log_trace (const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    log_msg (LOG_TRACE, fmt, args);
    va_end (args);
    fprintf (logfile, "\n");
}

void
log_warn (const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    log_msg (LOG_WARN, fmt, args);
    va_end (args);
    fprintf (logfile, "\n");
}

void
log_errno (const char *fmt, ...)
{
    int e = errno;
    va_list args;
    va_start (args, fmt);
    log_msg (LOG_ERROR, fmt, args);
    va_end (args);
    fprintf (logfile, ": %s\n", strerror (e));
}

void
logger_init (const prog_cfg *cfg)
{
    logfile = cfg->log_file;
}
