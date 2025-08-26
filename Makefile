CC       := gcc
STRIP    := strip --strip-all

SRC_DIR  := src
INC_DIR  := include
OBJ_DIR  := obj

CPPFLAGS := -I$(INC_DIR)

CFLAGS  := -Wall -Wextra -Wno-missing-braces -Os \
           -ffunction-sections -fdata-sections \
           -fomit-frame-pointer \
           -fno-unwind-tables -fno-asynchronous-unwind-tables \
           -flto

LDFLAGS := -Wl,--gc-sections -Wl,-s -flto -lm

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET   := color
TEST_SRC := tests/tests.c
TEST_OBJ := $(OBJ_DIR)/tests.o
TEST_BIN := $(TARGET)_test

GIT_HASH   := $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
GIT_BRANCH := $(shell git rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown)
COMPILE_TIME := $(shell date -u +"%Y-%m-%dT%H:%M:%SZ")

VERSION_FLAGS := -DGIT_HASH=\"$(GIT_HASH)\" -DGIT_BRANCH=\"$(GIT_BRANCH)\" -DCOMPILE_TIME=\"$(COMPILE_TIME)\"
CFLAGS += $(VERSION_FLAGS)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	-$(STRIP) $@ || true

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_OBJ): $(TEST_SRC) | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

PARSER_OBJS := $(filter-out $(OBJ_DIR)/main.o, $(OBJS))

$(TEST_BIN): $(PARSER_OBJS) $(TEST_OBJ)
	$(CC) $(PARSER_OBJS) $(TEST_OBJ) -o $@ $(LDFLAGS)
	-$(STRIP) $@ || true

test: $(TEST_BIN)
	@./$(TEST_BIN)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

targetname:
	@printf '%s\n' $(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TEST_BIN)

size: $(TARGET)
	@command -v size >/dev/null 2>&1 && size $(TARGET) || echo "size tool not found"

.PHONY: all targetname clean test size
