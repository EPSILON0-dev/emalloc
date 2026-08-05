TARGET = libemalloc
TEST_TARGET = test
VARIANT ?= RELEASE

TOOLCHAIN_PREFIX ?=
CC ?= $(TOOLCHAIN_PREFIX)gcc

SOURCES = emalloc.c malloc.c ebrk.c eslab.c eutil.c ebuddy.c emmap.c espinlock.c eapi.c
OBJECTS = $(SOURCES:.c=.o)
MAPFILE = exports.map

ifeq ($(VARIANT), RELEASE)
	OPTFLAGS = -O2
else
	OPTFLAGS = -g -O0
endif

CFLAGS = -Wall -Wextra -fno-builtin-malloc -fno-builtin
LDFLAGS = -fPIC

$(TARGET).so: $(OBJECTS)
	$(CC) -shared -Wl,--version-script=$(MAPFILE) $(OPTFLAGS) $(LDFLAGS) $(OBJECTS) -o $(TARGET).so

$(OBJECTS): %.o: %.c
	$(CC) -c $(OPTFLAGS) $(CFLAGS) $< -o $@

build: $(TARGET).so

clean:
	rm -f $(TARGET).so $(OBJECTS)

fresh: clean build

.PHONY: build clean fresh