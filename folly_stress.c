#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

/* Level of preprocessor indirection here so that macros passed to `STRINGIFY` will get expanded first */
#define STRINGIFY(x) STRINGIFY_HELPER(x)
#define STRINGIFY_HELPER(x) #x

#ifndef CHUNK_SIZE_DFL
#define CHUNK_SIZE_DFL 16384
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

void print_help(const char* const, FILE*);
int parse_pos_int(const char*, int*);

int main(int argc, char* argv[]) {
	int opt,
			num_chunks = NUM_CHUNKS_DFL,
			chunk_size = CHUNK_SIZE_DFL,
			iterations = ITERATIONS_DFL;

	malloctime_t malloc_time = mt_UNSET;

	/* Handle CLI args */
	static struct option long_options[] = {
		{"help",       no_argument,       NULL, 'h'},
		{"malloc",     required_argument, NULL, 'm'},
		{"num-chunks", optional_argument, NULL, 'n'},
		{"chunk-size", optional_argument, NULL, 's'},
		{"iterations", optional_argument, NULL, 'i'},
		{}
	};

	while ((opt = getopt_long(argc, argv, "hm:n:s:i:", long_options, NULL)) != -1) {
		switch (opt) {
			case 'h':
				print_help(argv[0], stdout);
				exit(EXIT_SUCCESS);
				break;
			case 'm': {
				char* time_req = optarg;
				if (!strncmp("before", time_req, sizeof("before"))) {
					malloc_time = mt_BEFORE;
				} else if (!strncmp("after", time_req, sizeof("after"))) {
					malloc_time = mt_AFTER;
				}
				break;
			}
			case 'n':
				if (!parse_pos_int(optarg, &num_chunks)) {
					fprintf(stderr, "Failed to parse \"%s\" as a positive integer\n", optarg);
					print_help(argv[0], stderr);
					exit(EXIT_FAILURE);
				}
				break;
			case 's':
				if (!parse_pos_int(optarg, &chunk_size)) {
					fprintf(stderr, "Failed to parse \"%s\" as a positive integer\n", optarg);
					print_help(argv[0], stderr);
					exit(EXIT_FAILURE);
				}
				break;
			case 'i':
				if (!parse_pos_int(optarg, &iterations)) {
					fprintf(stderr, "Failed to parse \"%s\" as a positive integer\n", optarg);
					print_help(argv[0], stderr);
					exit(EXIT_FAILURE);
				}
				break;
		}
	}

	if (malloc_time == mt_UNSET) {
		print_help(argv[0], stderr);
		exit(EXIT_FAILURE);
	}
}

void print_help(const char* const prog_name, FILE* stream) {
	fprintf(stream, "Usage: %s <mandatory args> [optional args]\n", prog_name);
	fputs("   Program to stress test folly\n\n", stream);
	fprintf(stream, "   Mandatory Args\n");
	fprintf(stream, "     %-27s%-52s", "-m,--malloc before|after", "whether to make malloc calls before or after fork");
	fputs("\n", stream);
	fprintf(stream, "   Optional Args\n");
	fprintf(stream, "     %-27s%-52s", "-h,--help", "print this help message");
	fputs("\n", stream);
	fprintf(stream, "     %-27s%-52s", "-n,--num-chunks[=N]", "number of chunks to allocate (default: " 
	       STRINGIFY(NUM_CHUNKS_DFL) ")");
	fputs("\n", stream);
	fprintf(stream, "     %-27s%-52s", "-s,--chunk-size[=N]", "size of each chunk in bytes (default: " 
	       STRINGIFY(CHUNK_SIZE_DFL) ")");
	fputs("\n", stream);
	fprintf(stream, "     %-27s%-52s", "-i,--iterations[=N]", "size of each chunk in bytes (default: " 
	       STRINGIFY(ITERATIONS_DFL) ")");
	fputs("\n", stream);
}

/// Returns 1 on successful conversion, 0 on failure.
/// Sets `out` to positive integer representation of `s`.
int parse_pos_int(const char* s, int* out) {
	// Necessary since calling a long optional argument with a ' ' instead of a '=' before the additional argument causes `s` to be NULL
	if (!s) {
		return 0;
	}

	errno = 0; /* Clear before `strtol` call */
	const char* endptr;
	long conversion = strtol(s, (char**)&endptr, 10);

	if (errno != 0 || endptr == s || *endptr != '\0' || conversion > INT_MAX || conversion < INT_MIN || conversion < 0) {
		return 0;
	}

	*out = (int)conversion;
	return 1;
}

