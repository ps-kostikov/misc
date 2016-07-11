#!/bin/bash

EXTRA=' a.deb b=1.2.3 c.lst'
echo $EXTRA
echo

for PKG in $EXTRA; do
    echo $PKG
    if echo "$PKG" | grep -q '.lst$'; then
        if [ -r "$PKG" ]; then
            for ITEM in $(cat $PKG); do
                # echo $ITEM
                NEW_EXTRA="$NEW_EXTRA $ITEM"
            done
        else
            echo "Can not read $PKG"
            exit 1
        fi
    else
        NEW_EXTRA="$NEW_EXTRA $PKG"
    fi
done

echo
echo $NEW_EXTRA