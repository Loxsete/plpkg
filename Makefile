CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -Iinclude
TARGET  = asd

SRCS    = $(wildcard src/*.c)
OBJS    = $(patsubst src/%.c, build/%.o, $(SRCS))

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	install -Dm755 $(TARGET) /usr/local/bin/$(TARGET)
	install -Dm644 asd.conf /etc/asd/asd.conf

clean:
	rm -rf build $(TARGET)

.PHONY: install clean
