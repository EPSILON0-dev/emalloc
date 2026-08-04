VARIANT ?= RELEASE
CFLAGS = -Wall -Wextra
LDFLAGS =

ifeq ($(VARIANT), RELEASE)
	OPTFLAGS = -O2
else
	OPTFLAGS = -g -O0
endif

TARGET = run_trace
SOURCES = run_trace.c
OBJECTS = $(SOURCES:.c=.o)

TRACES_DIR = traces
TRACE_FILES = $(TRACES_DIR)/random_trace1.rep $(TRACES_DIR)/random_trace2.rep $(TRACES_DIR)/random_trace3.rep

$(TARGET): $(OBJECTS) $(EMALLOC_LIB)
	gcc $(OPTFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(OBJECTS): %.o: %.c
	gcc -c $(OPTFLAGS) $(CFLAGS) $< -o $@

build: $(TARGET)

generate-traces: $(TRACES_DIR)
	python3 generate_trace.py -n 50 -s 16 -m 8192 -o $(TRACES_DIR)/random_slab_trace1.rep
	python3 generate_trace.py -n 150 -s 16 -m 8192 -o $(TRACES_DIR)/random_slab_trace2.rep
	python3 generate_trace.py -n 500 -s 16 -m 8192 -o $(TRACES_DIR)/random_slab_trace3.rep
	python3 generate_trace.py -n 2000 -s 16 -m 8192 -o $(TRACES_DIR)/random_slab_trace4.rep
	python3 generate_trace.py -n 10000 -s 16 -m 8192 -o $(TRACES_DIR)/random_slab_trace5.rep
	python3 generate_trace.py -n 100000 -s 16 -m 8192 -o $(TRACES_DIR)/random_slab_trace6.rep

	python3 generate_trace.py -n 50 -s 8193 -m 1048576 -o $(TRACES_DIR)/random_buddy_trace1.rep
	python3 generate_trace.py -n 150 -s 8193 -m 1048576 -o $(TRACES_DIR)/random_buddy_trace2.rep
	python3 generate_trace.py -n 500 -s 8193 -m 1048576 -o $(TRACES_DIR)/random_buddy_trace3.rep
	python3 generate_trace.py -n 2000 -s 8193 -m 1048576 -o $(TRACES_DIR)/random_buddy_trace4.rep
	python3 generate_trace.py -n 25000 -s 8193 -m 1048576 -o $(TRACES_DIR)/random_buddy_trace5.rep
	python3 generate_trace.py -n 100000 -s 8193 -m 131072 -o $(TRACES_DIR)/random_buddy_trace6.rep

	python3 generate_trace.py -n 50 -s 16 -m 262144 -o $(TRACES_DIR)/random_mixed_trace1.rep
	python3 generate_trace.py -n 150 -s 16 -m 262144 -o $(TRACES_DIR)/random_mixed_trace2.rep
	python3 generate_trace.py -n 500 -s 16 -m 262144 -o $(TRACES_DIR)/random_mixed_trace3.rep
	python3 generate_trace.py -n 2000 -s 16 -m 262144 -o $(TRACES_DIR)/random_mixed_trace4.rep
	python3 generate_trace.py -n 25000 -s 16 -m 262144 -o $(TRACES_DIR)/random_mixed_trace5.rep
	python3 generate_trace.py -n 100000 -s 16 -m 262144 -o $(TRACES_DIR)/random_mixed_trace6.rep

$(TRACES_DIR):
	mkdir -p $(TRACES_DIR)

clean:
	rm -f $(TARGET) $(OBJECTS)

clean-traces:
	rm -rf $(TRACES_DIR)

fresh: clean build

distclean: clean clean-traces

.PHONY: build generate-traces clean clean-traces distclean
