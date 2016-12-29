#!/bin/bash

host=$1
ipv6=$(host $host | grep IPv6 | awk '{print $5}')

echo $host
echo $ipv6

ssh-keygen -R $host
ssh-keygen -R $ipv6
ssh-keygen -R $host,$ipv6
ssh-keyscan -H $host,$ipv6 >> ~/.ssh/known_hosts
ssh-keyscan -H $ipv6 >> ~/.ssh/known_hosts
ssh-keyscan -H $host >> ~/.ssh/known_hosts