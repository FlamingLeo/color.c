CC       := gcc

STRIP    := strip --strip-all

SRC_DIR  := src
INC_DIR  := include
OBJ_DIR  := obj

CPPFLAGS := -I$(INC_DIR)

GIT_HASH   := $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
GIT_BRANCH := $(shell git rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown)
COMPILE_TIME := $(shell date -u +"%Y-%m-%dT%H:%M:%SZ")

VERSION_FLAGS := -DGIT_HASH=\"$(GIT_HASH)\" -DGIT_BRANCH=\"$(GIT_BRANCH)\" -DCOMPILE_TIME=\"$(COMPILE_TIME)\"

CFLAGS_COMMON := -Wall -Wextra -Wno-missing-braces

CFLAGS_RELEASE := -Os \
                  -ffunction-sections -fdata-sections \
                  -fomit-frame-pointer \
                  -fno-unwind-tables -fno-asynchronous-unwind-tables \
                  -flto

CFLAGS_DEBUG := -g -O0 -Og \
                 -fno-omit-frame-pointer \
                 -fno-unwind-tables -fno-asynchronous-unwind-tables \
                 -fno-lto -DDEBUG

LDFLAGS_COMMON := -Wl,--gc-sections
LDFLAGS_RELEASE := -Wl,-s -flto -lm
LDFLAGS_DEBUG := -lm

DEBUG ?= 0

ifeq ($(DEBUG),1)
CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_DEBUG) $(VERSION_FLAGS)
LDFLAGS := $(LDFLAGS_COMMON) $(LDFLAGS_DEBUG)
STRIP := true
else
CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_RELEASE) $(VERSION_FLAGS)
LDFLAGS := $(LDFLAGS_COMMON) $(LDFLAGS_RELEASE)
endif

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET   := color
TEST_SRC := tests/tests.c
TEST_OBJ := $(OBJ_DIR)/tests.o
TEST_BIN := $(TARGET)_test

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

.PHONY: all targetname clean test debug

debug:
	@$(MAKE) DEBUG=1 all
