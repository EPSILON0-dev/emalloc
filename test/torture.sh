#!/bin/sh

copy_failing_trace()
{
    trace=$1
    mkdir -p failing_traces
    cp $trace failing_traces/failing_trace_`ls failing_traces | wc -l`.rep
}

while true; do 
    python3 generate_trace.py -n 25000 -s 8192 -m 1000000 -o trace_candidate.rep
    ./run_trace ../libemalloc.so trace_candidate.rep
    exit_code=$?
    if [ $exit_code -ne 0 ]; then
        echo "### TRACE FAILED ###"
        copy_failing_trace trace_candidate.rep
    fi
done