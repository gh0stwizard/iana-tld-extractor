IDNKIT_DIR ?= /usr/local
IDNKIT_CFLAGS ?= -I$(IDNKIT_DIR)/include
IDNKIT_LIBS ?= -L$(IDNKIT_DIR)/lib -lidnkit

MYHTML_CFLAGS = -Imyhtml/include
MYHTML_LIBS = -Lmyhtml/lib -lmyhtml
MYHTML_LIBS_STATIC = $(MYHTML_LIBS)_static

CURL_MODS = libcurl
CURL_CFLAGS = $(shell pkg-config --cflags $(CURL_MODS))
CURL_LIBS = $(shell pkg-config --libs $(CURL_MODS))
CURL_LIBS_STATIC = $(shell pkg-config --static --libs $(CURL_MODS))

CFLAGS ?= 
CFLAGS += -Wall -std=c99 -pedantic
CFLAGS += -DHAVE_CURL
CFLAGS += $(IDNKIT_CFLAGS) $(MYHTML_CFLAGS)
CFLAGS += -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
LDFLAGS ?=
LIBS ?= 
LIBS += $(IDNKIT_LIBS) $(MYHTML_LIBS)
LIBS += $(CURL_LIBS)
LIBS_STATIC = $(IDNKIT_LIBS) $(MYHTML_LIBS_STATIC)
LIBS_STATIC += $(CURL_LIBS_STATIC)

TARGET = iana-tld-extractor
SOURCES = $(wildcard *.c)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

all: myhtml $(TARGET)

debug: CFLAGS += -g -D_DEBUG
debug: all

$(TARGET): $(OBJECTS)
	# shared linkage
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

distclean: clean myhtml-clean

clean:
	# cleanup
	$(RM) $(TARGET) $(OBJECTS)

strip: $(TARGET)
	# strip
	strip --strip-unneeded -R .comment -R .note -R .note.ABI-tag $(TARGET)

myhtml:
	# myhtml-library
	cd myhtml; $(MAKE) library

myhtml-clean:
	# myhtml-clean
	cd myhtml; $(MAKE) clean; rm -f myhtml.pc

.PHONY: all distclean clean debug strip myhtml
