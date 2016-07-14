#!/bin/bash +x

HOST=$1
IP=$(host $HOST | grep IPv6 | awk '{print $5}')
for ITEM in $HOST $IP; do 
    ssh-keygen -f ~/.ssh/known_hosts -R $ITEM
done