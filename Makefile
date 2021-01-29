CFLAGS ?= -O2
PKG_CONFIG ?= pkg-config
RM ?= rm -f
WITH_CURL ?= YES

CPPFLAGS = -Wall -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
CPPFLAGS += -Imyhtml/include

MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 0
VERSION = $(MAJOR_VERSION).$(MINOR_VERSION).$(PATCH_VERSION)
CPPFLAGS += -DAPP_VERSION=$(VERSION)

ifeq ($(WITH_CURL),YES)
CPPFLAGS += -DHAVE_CURL
CURL_CFLAGS = $(shell pkg-config --cflags libcurl)
CURL_LIBS = $(shell pkg-config --libs libcurl)
CURL_LIBS_STATIC = $(shell pkg-config --static --libs libcurl)
endif

ifdef FORCE_IDN
ifeq ($(FORCE_IDN),idnkit)
# idnkit
IDNKIT_DIR ?= /usr/local
DEFS ?= -I$(IDNKIT_DIR)/include
LIBS ?= -L$(IDNKIT_DIR)/lib -lidnkit
LIBS_STATIC ?= -L$(IDNKIT_DIR)/lib -lidnkit
else ifeq ($(FORCE_IDN),idn2)
# libidn2
CPPFLAGS += -DHAVE_IDN2
DEFS ?= $(shell $(PKG_CONFIG) --cflags libidn2)
LIBS ?= $(shell $(PKG_CONFIG) --libs libidn2)
LIBS_STATIC ?= $(shell $(PKG_CONFIG) --static --libs libidn2)
else
$(error Incorrect FORCE_IDN option. Valid values are: idnkit, idn2.)
endif
endif

MYHTML_CFLAGS = -Imyhtml/include
MYHTML_LIBS = -Lmyhtml/lib -lmyhtml
MYHTML_LIBS_STATIC = $(MYHTML_LIBS)_static -lpthread

TARGET = iana-tld-extractor
SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

#----------------------------------------------------------#

CPPFLAGS += $(DEFS) $(INCLUDES)

#----------------------------------------------------------#

all: $(TARGET)

debug: CFLAGS += -g -D_DEBUG
debug: all

$(TARGET): myhtml $(OBJECTS)
	# shared linkage
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) \
	$(LIBS) $(MYHTML_LIBS) $(CURL_LIBS)

static: myhtml $(OBJECTS)
	# static linkage
	$(CC) -static $(LDFLAGS) -o $(TARGET) $(OBJECTS) \
	$(LIBS_STATIC) $(MYHTML_LIBS_STATIC) $(CURL_LIBS_STATIC)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

distclean: clean myhtml-clean

clean:
	# cleanup
	$(RM) $(TARGET) $(OBJECTS)

strip: $(TARGET)
	# strip
	strip --strip-unneeded -R .comment -R .note -R .note.ABI-tag $(TARGET)

strip-static: static
	# strip (static)
	strip --strip-unneeded -R .comment -R .note -R .note.ABI-tag $(TARGET)

myhtml:
	# myhtml
	# XXX build with target `library` produce bad myhtml_static.a archive
	$(MAKE) -C myhtml

myhtml-clean:
	# myhtml-clean
	$(MAKE) -C myhtml clean
	$(RM) myhtml/myhtml.pc

.PHONY: all distclean clean debug strip myhtml
