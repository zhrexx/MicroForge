CC = gcc
CFLAGS = -Wall -Wextra
AR = ar
RM = rm -f
INSTALL = install
PREFIX = /usr/local
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

.PHONY: all clean install uninstall

all: libcrossline.a libcrossline.so

crossline.o: crossline.c crossline.h
	$(CC) $(CFLAGS) -c -o $@ $<

crossline_pic.o: crossline.c crossline.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

libcrossline.a: crossline.o
	$(AR) rcs $@ $^

libcrossline.so: crossline_pic.o
	$(CC) -shared -o $@ $^

example_static: example.c libcrossline.a
	$(CC) $(CFLAGS) $< -L. -lcrossline -o $@ -static

example_shared: example.c libcrossline.so
	$(CC) $(CFLAGS) $< -L. -lcrossline -o $@

clean:
	$(RM) *.o *.a *.so example_static example_shared

install: libcrossline.a libcrossline.so
	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 644 libcrossline.a $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 644 libcrossline.so $(DESTDIR)$(LIBDIR)
	$(INSTALL) -d $(DESTDIR)$(INCLUDEDIR)
	$(INSTALL) -m 644 crossline.h $(DESTDIR)$(INCLUDEDIR)

uninstall:
	$(RM) $(DESTDIR)$(LIBDIR)/libcrossline.a
	$(RM) $(DESTDIR)$(LIBDIR)/libcrossline.so
	$(RM) $(DESTDIR)$(INCLUDEDIR)/crossline.h
