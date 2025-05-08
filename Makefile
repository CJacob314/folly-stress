CC = gcc
INCLUDE_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src
# See here for info on `-D_POSIX_C_SOURCE=200809L`: https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html#index-_005fPOSIX_005fC_005fSOURCE
CFLAGS = -D_POSIX_C_SOURCE=200809L -O3 -g3 -I$(INCLUDE_DIR) -Wall -Wextra -Wpedantic -Werror -Wshadow -Wformat=2 -Wconversion -Wunused-parameter -std=c23
.DEFAULT_GOAL := all
.PHONY: clean clean_all compile_commands.json

$(BIN_DIR)/%: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Make sure $(BIN_DIR) exists
$(BIN_DIR):
	mkdir -p $@

all: $(BIN_DIR)/folly_stress

compile_commands.json:
	$(MAKE) clean
	bear -- $(MAKE)

clean:
	$(RM) -r $(BIN_DIR)

clean_all:
	$(RM) -r $(BIN_DIR) compile_commands.json
