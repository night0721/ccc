.POSIX:
.SUFFIXES:

TARGET = ggg
MANPAGE = ggg.1
SRC = ggg.c

CONF = config.h
DEFCONF = config.def.h
PREFIX ?= /usr/local

BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1

# Flags
CFLAGS += -O3 -s -std=c99 -W -pedantic -Wall -Wextra -Werror

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
