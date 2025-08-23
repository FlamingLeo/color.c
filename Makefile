CC      = gcc
CFLAGS  = -Wall -Wextra -Wno-missing-braces -I$(INC_DIR) -Os
LDFLAGS = -lm

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET   = color
TEST_SRC = tests/tests.c
TEST_OBJ = $(OBJ_DIR)/tests.o
TEST_BIN = $(TARGET)_test

GIT_HASH      := $(shell git rev-parse --short HEAD)
GIT_BRANCH    := $(shell git rev-parse --abbrev-ref HEAD)
COMPILE_TIME  := $(shell date -u)
VERSION_FLAGS := -DGIT_HASH=\"$(GIT_HASH)\" -DGIT_BRANCH="\"$(GIT_BRANCH)\"" -DCOMPILE_TIME="\"$(COMPILE_TIME)\""
CFLAGS += $(VERSION_FLAGS)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_OBJ): $(TEST_SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

PARSER_OBJS := $(filter-out $(OBJ_DIR)/main.o, $(OBJS))

$(TEST_BIN): $(PARSER_OBJS) $(TEST_OBJ)
	$(CC) $(PARSER_OBJS) $(TEST_OBJ) -o $@ $(LDFLAGS)

test: $(TEST_BIN)
	@./$(TEST_BIN)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

targetname:
	@printf '%s\n' $(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TEST_BIN)

.PHONY: all targetname clean test
