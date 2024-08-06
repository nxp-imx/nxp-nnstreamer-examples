/**
 * Copyright 2023-2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef CPP_LOGGING_H_
#define CPP_LOGGING_H_

/* Define macro for logging */
#define LOG_INFO true

#if LOG_INFO
#define log_info(...) printf("\n\033[1;32mINFO:\033[0m "); printf(__VA_ARGS__); fflush(stdout);

#else 
#define log_info(...)
#endif

#define LOG_ERROR true

#if LOG_ERROR
#define log_error(...) printf("\n\033[1;31mERROR:\033[0m "); printf(__VA_ARGS__); fflush(stdout);
#else 
#define log_error(...)
#endif

#define LOG_DEBUG true

#if LOG_DEBUG
#define log_debug(...) printf("\n\033[1;33mDEBUG:\033[0m "); printf(__VA_ARGS__); fflush(stdout);
#else 
#define log_debug(...)
#endif

#endif