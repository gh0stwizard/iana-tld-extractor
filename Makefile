CFLAGS ?= -O2
PKG_CONFIG ?= pkg-config
RM ?= rm -f
IDNKIT_DIR ?= /usr/local
WITH_CURL ?= YES
FORCE_IDN ?= idn2
WITH_STATIC_MYHTML ?= YES
PATCH ?= patch

CPPFLAGS = -Wall -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
CPPFLAGS += -Imyhtml/include

MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 2
VERSION = $(MAJOR_VERSION).$(MINOR_VERSION).$(PATCH_VERSION)

#----------------------------------------------------------#

ifeq ($(WITH_CURL),YES)
CPPFLAGS += -DHAVE_CURL
CURL_CFLAGS = $(shell pkg-config --cflags libcurl)
CURL_LIBS = $(shell pkg-config --libs libcurl)
CURL_LIBS_STATIC = $(shell pkg-config --static --libs libcurl)
endif

ifdef FORCE_IDN
ifeq ($(FORCE_IDN),idnkit)
# idnkit
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
else
# use idnkit by default
DEFS ?= -I$(IDNKIT_DIR)/include
LIBS ?= -L$(IDNKIT_DIR)/lib -lidnkit
LIBS_STATIC ?= -L$(IDNKIT_DIR)/lib -lidnkit
endif

MYHTML_CFLAGS = -Imyhtml/include
MYHTML_LIBS = -Lmyhtml/lib -lmyhtml
ifeq ($(WITH_STATIC_MYHTML),YES)
MYHTML_LIBS = -Wl,-Bstatic -Lmyhtml/lib -lmyhtml_static -Wl,-Bdynamic -lpthread
endif
MYHTML_LIBS_STATIC = -Lmyhtml/lib -lmyhtml_static

TARGET = iana-tld-extractor
SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

#----------------------------------------------------------#

CPPFLAGS += $(DEFS) $(INCLUDES) -DAPP_VERSION=$(VERSION)

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

# The bug was found on CentOS 6 with
# % ar V
# GNU ar version 2.20.51.0.2-5.48.el6_10.1 20100205
# [...]
#
# The case is when "ar crus" updating .a file, you see next:
# 1) at first call "make -C myhtml":
# % nm myhtml/lib/libmyhtml_static.a | grep mycore_string_realloc
# 0000000000000230 T mycore_string_realloc
#                  U mycore_string_realloc
#                  U mycore_string_realloc
#                  U mycore_string_realloc
#
# 2) at second call "make -C myhtml" the output would be:
# % nm myhtml/lib/libmyhtml_static.a | grep mycore_string_realloc
#                  U mycore_string_realloc
#                  U mycore_string_realloc
#                  U mycore_string_realloc
#                  U mycore_string_realloc
#
# Note that the text section (T mycore_string_realloc) is missing now.
# We forbid updating the archive with "u" option as a workaround.
# The new command will be "ar crs", instead of "ar crus".
myhtml-fix-ar:
	$(PATCH) -N -r - -p1 myhtml/Makefile misc/myhtml_gnu_ar.diff

myhtml-clean:
	# myhtml-clean
	$(MAKE) -C myhtml clean
	$(RM) myhtml/myhtml.pc

help:
	$(info Options:)
	$(info WITH_CURL = YES|NO - build with curl, default: YES)
	$(info FORCE_IDN = idnkit|idn2 - build with idnkit or libidn2, default: idn2)
	$(info WITH_STATIC_MYHTML = YES|NO - build statically myhtml, default: YES)
	$(info IDNKIT_DIR = /path/to/idnkit - idnkit install path, default: /usr/local)

.SILENT: help
.PHONY: all distclean clean debug strip myhtml
