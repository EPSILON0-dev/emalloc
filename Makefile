TARGET = emalloc
TEST_TARGET = test
VARIANT ?= RELEASE

SOURCES = emalloc.c malloc.c ebrk.c eslab.c eutil.c ebuddy.c
OBJECTS = $(SOURCES:.c=.o)

ifeq ($(VARIANT), RELEASE)
	OPTFLAGS = -O2
else
	OPTFLAGS = -g -O0
endif

CFLAGS = -Wall -Wextra -fno-builtin-malloc -fno-builtin
LDFLAGS =

$(TARGET).so: $(OBJECTS)
	gcc -shared $(OPTFLAGS) $(LDFLAGS) $(OBJECTS) -o $(TARGET).so

$(OBJECTS): %.o: %.c
	gcc -c $(OPTFLAGS) $(CFLAGS) $< -o $@

build: $(TARGET).so

clean:
	rm -f $(TARGET).so $(OBJECTS)

fresh: clean build

.PHONY: build clean fresh