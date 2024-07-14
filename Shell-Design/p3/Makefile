CC=gcc
CFLAGS=-fsanitize=address -g -Wall -Werror

TARGET=mysh
SRC_DIR=testFolder/testCodeSource
BIN_DIR=testFolder/testCodeBinary
SOURCES=$(wildcard $(SRC_DIR)/*.c)
BINARIES=$(patsubst $(SRC_DIR)/%.c,%,$(SOURCES))

$(TARGET): mysh.o
	$(CC) -o $@ $^ $(CFLAGS)

$(BINARIES): %: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $<

all: $(TARGET) $(BINARIES)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

.PHONY: clean

clean:
	rm -f mysh.o $(TARGET)
	rm -f $(BIN_DIR)/*

$(BINARIES): | $(BIN_DIR)
