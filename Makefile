.POSIX:
.SUFFIXES:

TARGET = ccc
MANPAGE = ccc.1
SRC = ccc.c util.c

# Flags
LDFLAGS = $(shell pkg-config --libs ncurses)
CFLAGS = -march=native -mtune=native -O3 -pipe -O3 -s -std=c11 -W -pedantic $(shell pkg-config --cflags ncurses) -Wall -Wextra # -Werror
CC=cc
CONF = config.h
DEFCONF = config.def.h
PREFIX ?= /usr/local

BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1

.PHONY: all install uninstall clean

all: $(TARGET)

$(TARGET): $(CONF) $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $@

$(CONF):
	cp -v $(DEFCONF) $(CONF)

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
