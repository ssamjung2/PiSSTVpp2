/**
 * @file logging.h
 * @brief Centralized logging utilities for timestamped verbose output
 *
 * Provides consistent timestamped logging across all PiSSTVpp2 modules.
 * Timestamps are only added when verbose AND timestamp_logging are both enabled.
 *
 * @author PiSSTVpp2 Contributors
 * @date January 2026
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

/**
 * @brief Print verbose message with optional millisecond-precision timestamp
 *
 * Prints a formatted message to stdout only if verbose is enabled.
 * Prepends [HH:MM:SS.mmm] timestamp if both verbose and add_timestamp are enabled.
 *
 * **Usage:**
 * @code
 * log_verbose(verbose, timestamp_logging, "Processing pixel %d\n", i);
 * log_verbose(verbose, timestamp_logging, "   [OK] Done\n");
 * @endcode
 *
 * **Output Examples:**
 * - When verbose=1, add_timestamp=0:
 *   `Processing pixel 42\n`
 * - When verbose=1, add_timestamp=1:
 *   `[22:34:42.123] Processing pixel 42\n`
 * - When verbose=0:
 *   (no output)
 *
 * @param verbose If 0, no output. If non-zero, print message
 * @param add_timestamp If non-zero (and verbose is non-zero), prepend timestamp
 * @param format Printf-style format string
 * @param ... Variable arguments matching format string
 */
static inline void log_verbose(int verbose, int add_timestamp, const char *format, ...) {
    if (!verbose) return;
    
    if (add_timestamp) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct tm *local_time = localtime(&tv.tv_sec);
        int milliseconds = tv.tv_usec / 1000;
        printf("[%02d:%02d:%02d.%03d] ", 
               local_time->tm_hour, 
               local_time->tm_min, 
               local_time->tm_sec,
               milliseconds);
    }
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

#endif /* LOGGING_H */
