bin_PROGRAMS = privbind
lib_LTLIBRARIES = libprivbind.la

man1_MANS = privbind.1
EXTRA_DIST = $(MANS)

privbind_SOURCES = main.c
noinst_HEADERS = ipc.h stub.h

libprivbind_la_SOURCES = libprivbind.c
libprivbind_la_LDFLAGS=-release 0 -ldl
