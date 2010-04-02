sbin_PROGRAMS = privbind
pkglib_LTLIBRARIES = privbind.la
noinst_PROGRAMS = testbind

doc_DATA = README
man_MANS = privbind.1
EXTRA_DIST = $(man_MANS)

privbind_SOURCES = main.c
noinst_HEADERS = ipc.h stub.h

privbind_la_SOURCES = libprivbind.c
privbind_la_LDFLAGS=-ldl -module -avoid-version

testbind_SOURCES = test.c
testbind_LDADD = -lpthread

# Define the various "make test" targets
.PHONY: test test-su test-sudo

test: privbind$(EXEEXT) testbind$(EXEEXT) privbind.la
	if [ -z "$(TESTUSER)" ] ; then echo "Must specify TESTUSER to use in test" ; exit 1 ; fi
	./privbind -u$(TESTUSER) -l`pwd`/.libs/@PRELOADLIBNAME@ $(PRIVBIND_PARAMS) \
	    ./testbind $(TESTBIND_PARAMS)

test-su: privbind$(EXEEXT) testbind$(EXEEXT) privbind.la
	su -c "$(MAKE) test TESTUSER=$(USER) PRIVBIND_PARAMS='$(PRIVBIND_PARAMS)' \
	    TESTBIND_PARAMS='$(TESTBIND_PARAMS)'"

test-sudo: privbind$(EXEEXT) testbind$(EXEEXT) privbind.la
	sudo $(MAKE) test TESTUSER=$(USER) PRIVBIND_PARAMS='$(PRIVBIND_PARAMS)' \
	    TESTBIND_PARAMS='$(TESTBIND_PARAMS)'
