CC = gcc
CFLAGS = -Wall -Wextra
TARGET = itemsarz
JSON_TARGET = items.json
PREFIX = /

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $^ -ljson-c

install: $(TARGET)
	cp $(TARGET) $(PREFIX)bin/
	mkdir -p $(PREFIX)usr/share/$(TARGET)
	cp $(JSON_TARGET) $(PREFIX)usr/share/itemsarz/

uninstall:
	rm -f $(PREFIX)bin/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all install uninstall clean
