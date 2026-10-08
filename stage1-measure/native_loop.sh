#!/bin/bash
# Run the native solver back to back and print each run's wall time in ms.
cd ~/minirubik
for i in $(seq "${1:-30}"); do
    s=$(date +%s%N)
    ./solver 21345671111111 > /dev/null
    e=$(date +%s%N)
    echo $(( (e - s) / 1000000 ))
done
