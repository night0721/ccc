.POSIX:
.SUFFIXES:

TARGET = ccc
MANPAGE = ccc.1
SRC = ccc.c util.c file.c config.h

# Flags
LDFLAGS = $(shell pkg-config --libs ncurses)
CFLAGS = -march=native -mtune=native -O3 -pipe -s -std=c99 -pedantic $(shell pkg-config --cflags ncurses) -Wall # -Wextra -Werror
CC=cc
PREFIX ?= /usr/local
CONF = config.h

BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1

.PHONY: all install uninstall clean

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $@

all: $(TARGET)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)
	cp -p $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	chmod 755 $(DESTDIR)$(BINDIR)/$(TARGET)
	cp -p $(MANPAGE) $(DESTDIR)$(MANDIR)/$(MANPAGE)
	chmod 644 $(DESTDIR)$(MANDIR)/$(MANPAGE)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(TARGET)
	$(RM) $(DESTDIR)$(MANDIR)/$(MANPAGE)

clean:
	$(RM) $(TARGET)
