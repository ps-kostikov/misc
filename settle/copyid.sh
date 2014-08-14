#!/bin/bash

host=$1
username=pkostikov

ssh $username@$host mkdir -p .ssh
cat ~/.ssh/id_rsa.pub | ssh $username@$host 'cat >> .ssh/authorized_keys'
echo $host >> known_hosts


