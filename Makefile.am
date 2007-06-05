bin_PROGRAMS = privbind
pkglib_LTLIBRARIES = libprivbind.la
noinst_PROGRAMS = testbind

man1_MANS = privbind.1
EXTRA_DIST = $(MANS)

privbind_SOURCES = main.c
noinst_HEADERS = ipc.h stub.h

main.o: paths.h

libprivbind_la_SOURCES = libprivbind.c
libprivbind_la_LDFLAGS=-release 0 -ldl

testbind_SOURCES = test.c

# Record the directory for package libraries
CLEANFILES=paths.h

paths.h: Makefile
	echo "#define PKGLIBDIR \"$(pkglibdir)\"" > $@

# Define the various "make test" targets
.PHONY: test test-su test-sudo

test: privbind$(EXEEXT) testbind$(EXEEXT) libprivbind.la
	if [ -z "$(TESTUSER)" ] ; then echo "Must specify TESTUSER to use in test" ; exit 1 ; fi
	./privbind -u$(TESTUSER) -l`pwd`/.libs/@PRELOADLIBNAME@ $(PRIVBIND_PARAMS) \
	    ./testbind $(TESTBIND_PARAMS)

test-su: privbind$(EXEEXT) testbind$(EXEEXT) libprivbind.la
	su -c "$(MAKE) test TESTUSER=$(USER) PRIVBIND_PARAMS='$(PRIVBIND_PARAMS)' \
	    TESTBIND_PARAMS='$(TESTBIND_PARAMS)'"

test-sudo: privbind$(EXEEXT) testbind$(EXEEXT) libprivbind.la
	sudo $(MAKE) test TESTUSER=$(USER) PRIVBIND_PARAMS='$(PRIVBIND_PARAMS)' \
	    TESTBIND_PARAMS='$(TESTBIND_PARAMS)'
