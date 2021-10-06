PNAME=jpegrepair
PROGS=$(PNAME)
SRCS=.
CC=gcc
CFLAGS=-Wall -I$(SRCS)
ifeq ($(OS),Windows_NT)
  LIBS=libjpeg-8.dll
else
  LIBS=-ljpeg
endif
PREFIX?=/usr/local
DATAPREFIX=$(PREFIX)/share
INSTALL=install
RM=rm -fv

all: $(PROGS)

$(PNAME): $(SRCS)/$(PNAME).c $(SRCS)/transupp.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -s

clean:
	$(RM) $(PROGS)

install: $(PROGS)
	$(INSTALL) -d $(PREFIX)/bin
	$(INSTALL) -m 0755 $(PROGS) $(PREFIX)/bin/
	$(INSTALL) -d $(DATAPREFIX)
	$(INSTALL) -d $(DATAPREFIX)/man/man1
	$(INSTALL) -m 0644 man/man1/$(PNAME).1 $(DATAPREFIX)/man/man1
	$(INSTALL) -d $(DATAPREFIX)/doc/$(PNAME)
	$(INSTALL) -m 0644 LICENSE README.md README.ijg $(DATAPREFIX)/doc/$(PNAME)
