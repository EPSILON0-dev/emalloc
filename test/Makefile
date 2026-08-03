CFLAGS = -O2 -Wall -Wextra
LDFLAGS =

TARGET = run_trace
SOURCES = run_trace.c
OBJECTS = $(SOURCES:.c=.o)

# Generated trace files
TRACES_DIR = traces
TRACE_FILES = $(TRACES_DIR)/random_trace1.rep $(TRACES_DIR)/random_trace2.rep $(TRACES_DIR)/random_trace3.rep

$(TARGET): $(OBJECTS) $(EMALLOC_LIB)
	gcc $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(OBJECTS): %.o: %.c
	gcc -c $(CFLAGS) $< -o $@

build: $(TARGET)

generate-traces: $(TRACES_DIR)
	python3 generate_trace.py -n 50 -s 20000 -o $(TRACES_DIR)/random_trace1.rep
	python3 generate_trace.py -n 150 -s 50000 -o $(TRACES_DIR)/random_trace2.rep
	python3 generate_trace.py -n 500 -s 100000 -o $(TRACES_DIR)/random_trace3.rep
	python3 generate_trace.py -n 2000 -s 1000000 -o $(TRACES_DIR)/random_trace4.rep
	python3 generate_trace.py -n 10000 -s 10000000 -o $(TRACES_DIR)/random_trace5.rep

$(TRACES_DIR):
	mkdir -p $(TRACES_DIR)

clean:
	rm -f $(TARGET) $(OBJECTS)

clean-traces:
	rm -rf $(TRACES_DIR)

distclean: clean clean-traces

.PHONY: build generate-traces clean clean-traces distclean
