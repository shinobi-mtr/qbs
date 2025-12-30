CC      := clang
CFLAGS  := -Wall -Wextra
SRC_DIR := examples
OUT_DIR := out

SRCS := $(shell find $(SRC_DIR) -name "*.c")
BINS := $(patsubst $(SRC_DIR)/%.c, $(OUT_DIR)/%, $(SRCS))

all: $(BINS)

$(OUT_DIR)/%: $(SRC_DIR)/%.c
	@# Create the specific sub-folder in 'out' if it doesn't exist
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OUT_DIR)

.PHONY: all clean
