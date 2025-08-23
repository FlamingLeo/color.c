CC = gcc
CFLAGS = -Wall -Wextra -Wno-missing-braces -I$(INC_DIR) -Os
LDFLAGS = -lm

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = color
TEST_SRC = tests/tests.c
TEST_OBJ = $(OBJ_DIR)/tests.o
TEST_BIN = $(TARGET)_test

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
