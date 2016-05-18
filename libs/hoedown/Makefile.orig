CFLAGS = -g -O3 -ansi -pedantic -Wall -Wextra -Wno-unused-parameter
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

HOEDOWN_CFLAGS = $(CFLAGS) -Isrc
ifneq ($(OS),Windows_NT)
	HOEDOWN_CFLAGS += -fPIC
endif

SONAME = -soname
ifeq ($(shell uname -s),Darwin)
	SONAME = -install_name
endif

HOEDOWN_SRC=\
	src/autolink.o \
	src/buffer.o \
	src/document.o \
	src/escape.o \
	src/html.o \
	src/html_blocks.o \
	src/html_smartypants.o \
	src/stack.o \
	src/version.o

.PHONY:		all test test-pl clean

all:		libhoedown.so libhoedown.a hoedown smartypants

# Libraries

libhoedown.so: libhoedown.so.3
	ln -f -s $^ $@

libhoedown.so.3: $(HOEDOWN_SRC)
	$(CC) -Wl,$(SONAME),$(@F) -shared $^ $(LDFLAGS) -o $@

libhoedown.a: $(HOEDOWN_SRC)
	$(AR) rcs libhoedown.a $^

# Executables

hoedown: bin/hoedown.o $(HOEDOWN_SRC)
	$(CC) $^ $(LDFLAGS) -o $@

smartypants: bin/smartypants.o $(HOEDOWN_SRC)
	$(CC) $^ $(LDFLAGS) -o $@

# Perfect hashing

src/html_blocks.c: html_block_names.gperf
	gperf -L ANSI-C -N hoedown_find_block_tag -c -C -E -S 1 --ignore-case -m100 $^ > $@

# Testing

test: hoedown
	python test/runner.py

test-pl: hoedown
	perl test/MarkdownTest_1.0.3/MarkdownTest.pl \
		--script=./hoedown --testdir=test/MarkdownTest_1.0.3/Tests --tidy

# Housekeeping

clean:
	$(RM) src/*.o bin/*.o
	$(RM) libhoedown.so libhoedown.so.1 libhoedown.a
	$(RM) hoedown smartypants hoedown.exe smartypants.exe

# Installing

install:
	install -m755 -d $(DESTDIR)$(LIBDIR)
	install -m755 -d $(DESTDIR)$(BINDIR)
	install -m755 -d $(DESTDIR)$(INCLUDEDIR)

	install -m644 libhoedown.a $(DESTDIR)$(LIBDIR)
	install -m755 libhoedown.so.3 $(DESTDIR)$(LIBDIR)
	ln -f -s libhoedown.so.3 $(DESTDIR)$(LIBDIR)/libhoedown.so

	install -m755 hoedown $(DESTDIR)$(BINDIR)
	install -m755 smartypants $(DESTDIR)$(BINDIR)

	install -m755 -d $(DESTDIR)$(INCLUDEDIR)/hoedown
	install -m644 src/*.h $(DESTDIR)$(INCLUDEDIR)/hoedown

# Generic object compilations

%.o: %.c
	$(CC) $(HOEDOWN_CFLAGS) -c -o $@ $<

src/html_blocks.o: src/html_blocks.c
	$(CC) $(HOEDOWN_CFLAGS) -Wno-static-in-inline -c -o $@ $<
