TARGET = emalloc
TEST_TARGET = test

SOURCES = emalloc.c malloc.c ebrk.c eslab.c eutil.c
OBJECTS = $(SOURCES:.c=.o)

CFLAGS = -O2 -Wall -Wextra
LDFLAGS =

$(TARGET).so: $(OBJECTS)
	gcc -shared $(OBJECTS) -o $(TARGET).so

$(OBJECTS): %.o: %.c
	gcc -c $< -o $@

build: $(TARGET).so

clean:
	rm -f $(TARGET).so $(OBJECTS)

fresh: clean build

.PHONY: build clean fresh