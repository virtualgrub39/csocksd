#ifndef LOG_H
#define LOG_H

#include "config.h"

enum log_level
{
    LOG_TRACE,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
};

static const char *log_level_strings[] = {
    [LOG_TRACE] = "TRACE",
    [LOG_INFO] = "INFO",
    [LOG_WARN] = "WARN",
    [LOG_ERROR] = "ERROR",
};

void logger_init (const prog_cfg *cfg);
// void log_msg (enum log_level level, const char *fmt, ...);
void log_errno (const char *fmt, ...);
void log_error (const char *fmt, ...);
void log_info (const char *fmt, ...);
void log_trace (const char *fmt, ...);
void log_warn (const char *fmt, ...);

#endif
