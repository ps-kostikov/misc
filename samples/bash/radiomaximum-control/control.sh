#!/bin/bash -x

if [ "$1" == "OFF" ]
then
  curl -X POST -H 'Content-Type: application/json' -H 'Authorization: Bearer 7b57287ffd8fa756271c2d68cea6e9d05fb7162df550bf31b059df0820149f44' -d '{"type":"power_off"}' "https://api.digitalocean.com/v2/droplets/6880140/actions"
fi

if [ "$1" == "ON" ]
then
  curl -X POST -H 'Content-Type: application/json' -H 'Authorization: Bearer 7b57287ffd8fa756271c2d68cea6e9d05fb7162df550bf31b059df0820149f44' -d '{"type":"power_on"}' "https://api.digitalocean.com/v2/droplets/6880140/actions"
fi
