CC = gcc

CFLAGS = -Wall
# For Debugging
# CFLAGS = -Wall -Wextra -g
# For Strict warnings
# CFLAGS = -Wall -Wextra -Werror -pedantic -O2

SRC = $(shell find . -name "*.c")

TARGET = lb

all:
	$(CC) $(CFLAG) $(SRC) -o bin/$(TARGET)

run: all
	./bin/$(TARGET)

clean:
	rm -f $(TARGET)
