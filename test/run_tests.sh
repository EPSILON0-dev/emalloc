#!/bin/sh

copy_failing_trace()
{
    trace=$1
    mkdir -p failing_traces
    cp $trace failing_traces/failing_trace_`ls failing_traces | wc -l`.rep
}

for trace in $(ls traces/*.rep); do
    echo "Running trace: $trace"
    ./run_trace ../libemalloc.so $trace > /dev/null 2> /dev/null
    exit_code=$?
    if [ $exit_code -ne 0 ]; then
        echo "### TRACE FAILED ###"
        copy_failing_trace $trace
    fi
done