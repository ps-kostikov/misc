#!/bin/bash

for f in tmp/*.conf; do
    if [ -e "$f" ]; then
        # su ecstatic -c ". $f"
        . "$f"
    fi
done

echo $PP
echo $FF