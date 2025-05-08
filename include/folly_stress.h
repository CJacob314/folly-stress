#pragma once
#include <stdio.h>

/* Helper macros for calling functions which set errno and return negative numbers on error */
#define EXIT_IF_FAILS(call)                                                    \
  do {                                                                         \
    if ((call) < 0) {                                                          \
      perror(#call);                                                           \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#define EXIT_IF_FAILS_ASSIGN(var, call)                                        \
  do {                                                                         \
    var = (call);                                                              \
    if (var < 0) {                                                             \
      perror(#call);                                                           \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

/* Level of preprocessor indirection here so that macros passed to `STRINGIFY` will get expanded first */
#define STRINGIFY(x) STRINGIFY_HELPER(x)
#define STRINGIFY_HELPER(x) #x

#ifndef CHUNK_SIZE_DFL
#define CHUNK_SIZE_DFL 4096
#endif

#ifndef NUM_CHUNKS_DFL
#define NUM_CHUNKS_DFL 256
#endif

#ifndef ITERATIONS_DFL
#define ITERATIONS_DFL 20
#endif

typedef enum MallocTime {
	mt_UNSET, mt_BEFORE, mt_AFTER
} malloctime_t;

/* Some ANSI color codes */
#define ANSI_GREEN "\033[0;32m"
#define ANSI_RESET "\033[0m"

static void print_help(const char* const, FILE*);
static int parse_pos_int(const char* s, int* out);
static void do_stress(const malloctime_t malloc_time, const int num_chunks, const int chunk_size, const int iterations) __attribute__((noreturn));
