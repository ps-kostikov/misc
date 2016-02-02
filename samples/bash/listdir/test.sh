#!/bin/bash

if [ -d tmp3 ]; then
    for f in tmp3/*.conf
    do
        echo "Processing $f file..."
    done
fi