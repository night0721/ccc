.POSIX:
.SUFFIXES:

VERSION = 1.0
TARGET = ccc
MANPAGE = $(TARGET).1
CONF = config.h
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1

LDFLAGS != pkg-config --libs ncursesw
INCFLAGS != pkg-config --cflags ncursesw
CFLAGS = -O3 -march=native -mtune=native -pipe -s -flto -std=c99 -pedantic -Wall $(INCFLAGS)

SRC = ccc.c util.c file.c icons.c

$(TARGET): $(SRC) $(CONF)
	$(CC) $(SRC) -o $@ $(CFLAGS) $(LDFLAGS)

dist:
	mkdir -p $(TARGET)-$(VERSION)
	cp -R README.md $(MANPAGE) $(TARGET) $(TARGET)-$(VERSION)
	tar -cf $(TARGET)-$(VERSION).tar $(TARGET)-$(VERSION)
	gzip $(TARGET)-$(VERSION).tar
	rm -rf $(TARGET)-$(VERSION)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)
	cp -p $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	chmod 755 $(DESTDIR)$(BINDIR)/$(TARGET)
	cp -p $(MANPAGE) $(DESTDIR)$(MANDIR)/$(MANPAGE)
	chmod 644 $(DESTDIR)$(MANDIR)/$(MANPAGE)

uninstall:
	rm $(DESTDIR)$(BINDIR)/$(TARGET)
	rm $(DESTDIR)$(MANDIR)/$(MANPAGE)

clean:
	rm $(TARGET)

all: $(TARGET)

.PHONY: all dist install uninstall clean
