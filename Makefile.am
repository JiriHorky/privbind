bin_PROGRAMS = privbind
pkglib_LTLIBRARIES = libprivbind.la
noinst_PROGRAMS = test

man1_MANS = privbind.1
EXTRA_DIST = $(MANS)

privbind_SOURCES = main.c
noinst_HEADERS = ipc.h stub.h

main.c: paths.h

libprivbind_la_SOURCES = libprivbind.c
libprivbind_la_LDFLAGS=-release 0 -ldl

test_SOURCES = test.c

# Record the directory for package libraries
CLEANFILES=paths.h

paths.h: Makefile
	echo "#define PKGLIBDIR \"$(pkglibdir)\"" > $@
