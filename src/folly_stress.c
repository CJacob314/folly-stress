#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <sys/wait.h>
#include <folly_stress.h>

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
		{"num-chunks", required_argument, NULL, 'n'},
		{"chunk-size", required_argument, NULL, 's'},
		{"iterations", required_argument, NULL, 'i'},
		{0}
	};

	while ((opt = getopt_long(argc, argv, "hm:n:s:i:", long_options, NULL)) != -1) {
		switch (opt) {
			case 'h':
				print_help(argv[0], stdout);
				exit(EXIT_SUCCESS);
				break;
			case 'm': {
				char* time_req = optarg;
				if (!strcmp("before", time_req)) {
					malloc_time = mt_BEFORE;
				} else if (!strcmp("after", time_req)) {
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

	do_stress(malloc_time, num_chunks, chunk_size, iterations);
}

static void print_help(const char* const prog_name, FILE* stream) {
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
	fprintf(stream, "     %-27s%-52s", "-s,--chunk-size[=N]", "size of each chunk in C `int`s (default: " 
	       STRINGIFY(CHUNK_SIZE_DFL) ")");
	fputs("\n", stream);
	fprintf(stream, "     %-27s%-52s", "-i,--iterations[=N]", "stress test number of iterations (default: " 
	       STRINGIFY(ITERATIONS_DFL) ")");
	fputs("\n", stream);
}

/// Returns 1 on successful conversion, 0 on failure.
/// Sets `out` to positive integer representation of `s`.
static int parse_pos_int(const char* s, int* out) {
	errno = 0; /* Clear before `strtol` call */
	const char* endptr;
	long conversion = strtol(s, (char**)&endptr, 10);

	if (errno != 0 || endptr == s || *endptr != '\0' || conversion > INT_MAX || conversion < INT_MIN || conversion < 0) {
		return 0;
	}

	*out = (int)conversion;
	return 1;
}

static __attribute__((noreturn)) void do_stress(const malloctime_t malloc_time, const int num_chunks, const int chunk_size, const int iterations) {
	/* Print info about run */
	puts("Running with configuration:");
	printf("   %-12s%-25s\n", "malloc_time", malloc_time == mt_BEFORE ? "before" : "after");
	printf("   %-12s%-25d\n", "num_chunks", num_chunks);
	printf("   %-12s%-25d\n", "chunk_size", chunk_size);
	printf("   %-12s%-25d\n", "iterations", iterations);

	int descending = 0;
	pid_t pid;
	volatile unsigned int** chunks = malloc((size_t) num_chunks * sizeof(int*));
	if (!chunks) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	if (malloc_time == mt_BEFORE) {
		// Allocate chunks now, before fork
		for (int i = 0; i < num_chunks; ++i) {
			chunks[i] = malloc((size_t) chunk_size * sizeof(int));
			if (!chunks[i]) {
				perror("malloc");
				exit(EXIT_FAILURE);
			}
		}
	}

	EXIT_IF_FAILS_ASSIGN(pid, fork()); /* Fork */

	if (!pid) {
		/* Child will write descending integers; parent will write ascending */
		descending = 1;
	}

	if (malloc_time == mt_AFTER) {
		// Allocate chunks after fork
		for (int i = 0; i < num_chunks; ++i) {
			chunks[i] = malloc((size_t) chunk_size * sizeof(int));
			if (!chunks[i]) {
				perror("malloc");
				exit(EXIT_FAILURE);
			}
		}
	}

	/* Begin iterations */
	for (int iteration = 0; iteration < iterations; ++iteration) {
		/* Zero all chunks */
		for (int i = 0; i < num_chunks; ++i) {
			memset((void*) chunks[i], 0x0, (size_t) chunk_size * sizeof(int));
		}

		/* Write integers to each chunk */
		unsigned int integer = descending ? UINT_MAX : 0;
		for (int chunk_i = 0; chunk_i < num_chunks; ++chunk_i) {
			volatile unsigned int* chunk = chunks[chunk_i];
			for (int i = 0; i < chunk_size; ++i) {
				chunk[i] = descending ? integer-- : integer++;
			}
		}

		/* Verify all memory is as expected */
		integer = descending ? UINT_MAX : 0;
		for (int chunk_i = 0; chunk_i < num_chunks; ++chunk_i) {
			volatile unsigned int* chunk = chunks[chunk_i];
			for (int i = 0; i < chunk_size; ++i) {
				assert(chunk[i] == (descending ? integer-- : integer++));
			}
		}
	}
	
	
	/* Free chunks */
	for (int i = 0; i < num_chunks; ++i) {
		free((void*) chunks[i]);
	}
	free(chunks);
	if (pid) {
		/* Wait for child */
		int status;
		pid_t wait_ret;
		while (1) {
			wait_ret = waitpid(pid, &status, 0);
			if (wait_ret < 0) {
				perror("waitpid");
				exit(EXIT_FAILURE);
			}

			if (WIFEXITED(status)) {
				int exit_code = WEXITSTATUS(status);

				if (exit_code == 0) {
					puts(ANSI_GREEN "Tests passed" ANSI_RESET);
					break;
				}
			} else if (WIFSIGNALED(status)) {
				fprintf(stderr, "ERROR: Child process %d terminated by signal %d\n", pid, WTERMSIG(status));
				exit(EXIT_FAILURE);
			} else if (WIFSTOPPED(status)) {
				fprintf(stderr, "ERROR: Child process %d stopped by signal %d\n", pid, WSTOPSIG(status));
				exit(EXIT_FAILURE);
			} else {
				fprintf(stderr, "ERROR: Unknown error while waiting on child %d\n", pid);
				exit(EXIT_FAILURE);
			}
		}
	}
	exit(EXIT_SUCCESS);
}
