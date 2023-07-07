/**
 * Copyright 2023 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef LOGGING_HH
#define LOGGING_HH

/* Define macro for logging */
#define INFO true

#if INFO
#define log_info(...) printf("\n\033[1;32mINFO:\033[0m "); printf(__VA_ARGS__); fflush(stdout);

#else 
#define log_info(...)
#endif

#define ERROR true

#if ERROR
#define log_error(...) printf("\n\033[1;31mERROR:\033[0m "); printf(__VA_ARGS__); fflush(stdout);
#else 
#define log_error(...)
#endif

#define DEBUG false

#if DEBUG
#define log_debug(...) printf("\n\033[1;33mDEBUG:\033[0m "); printf(__VA_ARGS__); fflush(stdout);
#else 
#define log_debug(...)
#endif
#endif