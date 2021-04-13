/*
 * Copyright 2021 FUJIFILM Corporation
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file output_level.c
 * 
 * Output information according to verbose level.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "ltos_format_checker.h"

static const char* verbose_level = NULL; // DISPLAY_HEADER_INFO, DISPLAY_HEADER_AND_L4_INFO, ... DISPLAY_ALL_INFO
static const char* continue_mode = NULL; // CONT or not
static const char* top_verbose   = NULL;

/**
 * Set verbose level specified in command line.
 * @param [in] (vl) Verbose level option specified on the command line.
 */
void set_vl(const char* const vl) {
  verbose_level = vl;
}

/**
 * Set verbose level specified in each method.
 * @param [in] (t_verbose) Verbose level option specified in each method.
 */
void set_top_verbose(const char* const t_verbose) {
  top_verbose = t_verbose;
}

/**
 * Get verbose level specified in each method.
 * @param [in] (t_verbose) Verbose level option specified in each method.
 */
const char* get_top_verbose() {
  return top_verbose;
}

/**
 * Set continue mode.
 * @param [in] (c_mode) Continue mode option specified on the command line.
 */
void set_c_mode(const char* const c_mode) {
	continue_mode = c_mode;
}

/**
 * Get verbose rank to check a rank includes other rank or not.
 * @param [in] (verbose) String to show verbose.
 * @return               Verbose rank is returned. Bigger value means more verbose.
 */
static int get_verbose_rank(const char* const verbose) {
  int verbose_rank = 0;

  if (strcmp(verbose, DISPLAY_HEADER_INFO) == 0) {
    verbose_rank = 1;
  } else if (strcmp(verbose, DISPLAY_HEADER_AND_L4_INFO) == 0) {
    verbose_rank = 2;
  } else if (strcmp(verbose, DISPLAY_HEADER_AND_L43_INFO) == 0) {
    verbose_rank = 3;
  } else if (strcmp(verbose, DISPLAY_HEADER_AND_L432_INFO) == 0) {
    verbose_rank = 4;
  } else if (strcmp(verbose, DISPLAY_HEADER_AND_L4321_INFO) == 0) {
    verbose_rank = 5;
  } else if (strcmp(verbose, DISPLAY_ALL_INFO) == 0) {
    verbose_rank = 6;
  }

  return verbose_rank;
}

/**
 * Output header according to verbose level.
 * @param [out] (stream)    Stream to write header. stdout, stderr or log file.
 * @param [in]  (log_level) Log Level like Error, Warning, Info, Debug and Trace.
 * @param [in]  (verbose)   Verbose level like Label, RCM, PR, OCM, PO and MISC.
 */
static void print_header(FILE* const stream, const char* const log_level, const char* const verbose) {
  struct timeval tvToday; // for msec
  struct tm *ptm; // for date and time

  gettimeofday(&tvToday, NULL); // Today
  ptm = localtime(&tvToday.tv_sec);
  char char_date_and_time_now[MAX_PATH + 1] = { 0 };
  sprintf(char_date_and_time_now, "%04d/%02d/%02d %02d:%02d:%02d.%3d ",
      ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
      ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
      (uint16_t)(tvToday.tv_usec / 1000));
  fprintf(stream, char_date_and_time_now);

  fprintf(stream, log_level);

  if (!strcmp(verbose, DISPLAY_COMMON_INFO)) {
    fprintf(stream, "[COMMON] ");
  } else if (!strcmp(verbose, DISPLAY_HEADER_INFO)) {
    fprintf(stream, "[LABEL ] ");
  } else if (!strcmp(verbose ,DISPLAY_HEADER_AND_L4_INFO)) {
    fprintf(stream, "[RCM   ] ");
  } else if (!strcmp(verbose ,DISPLAY_HEADER_AND_L43_INFO)) {
    fprintf(stream, "[PR    ] ");
  } else if (!strcmp(verbose ,DISPLAY_HEADER_AND_L432_INFO)) {
    fprintf(stream, "[OCM   ] ");
  } else if (!strcmp(verbose ,DISPLAY_HEADER_AND_L4321_INFO)) {
    fprintf(stream, "[POINFO] ");
  } else if (!strcmp(verbose ,DISPLAY_ALL_INFO)) {
    fprintf(stream, "[MISC  ] ");
  } else {
    fprintf(stream, "[UNKNOWN] ");
  }
}

/**
 * Output information according to verbose level.
 * @param [in] (log_level) ERROR/WARNING/INFO/DEBUG/TRACE
 * @param [in] (verbose)   Verbose level specified in each method.
 * @param [in] (format)    Information to display.
 * @return     (OK/NG)     For Error message, return NG. Otherwise, return OK.
 */
int output_accdg_to_vl(const char* const log_level, const char* verbose, const char * restrict format, ...){
  int ret = OK;
  va_list args;

  if (top_verbose == NULL && !strcmp(verbose, DEFAULT)) {
    verbose = "UNKNOWN";
  }
  if (!strcmp(verbose, DEFAULT)) {
    verbose = top_verbose;
  }
  va_start(args, format);
  if (!strcmp(log_level, OUTPUT_ERROR) || !strcmp(log_level, OUTPUT_WARNING) || !strcmp(log_level, OUTPUT_SYSTEM_ERROR)) {
    fflush(stdout);
    print_header(stderr, log_level, verbose);
    vfprintf(stderr, format, args);
    fflush(stderr);

    if (!strcmp(log_level, OUTPUT_SYSTEM_ERROR)) {
      va_end(args);
      exit(1);
    } else if (!strcmp(log_level, OUTPUT_ERROR)) {
      if (strcmp(continue_mode, CONT) != 0) {
        va_end(args);
        exit(1);
      }
      ret = NG;
    }
  } else if (!strcmp(log_level, OUTPUT_INFO) || !strcmp(log_level, OUTPUT_DEBUG) || !strcmp(log_level, OUTPUT_TRACE)) {
    if (get_verbose_rank(verbose_level) >= get_verbose_rank(verbose)) {
      print_header(stdout, log_level, verbose);
      vprintf(format, args);
    }
  }
  va_end(args);
  return ret;
}
