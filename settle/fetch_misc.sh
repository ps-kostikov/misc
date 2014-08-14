#!/bin/bash

host=$1
username=pkostikov

echo $host

scp -r $username@$host:~/misc ~/Work/

