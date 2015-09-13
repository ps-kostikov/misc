#!/usr/bin/python

import sys
import requests
import json

HEADERS = {
    'Content-Type': 'application/json',
    'Authorization': 'Bearer 7b57287ffd8fa756271c2d68cea6e9d05fb7162df550bf31b059df0820149f44'
}
BASE_URL = 'https://api.digitalocean.com/v2'
NAME = 'radiomaximum'


def find_droplet():
    droplets = requests.get(BASE_URL + '/droplets', headers=HEADERS).json()['droplets']
    droplets = filter(lambda d: d['name'] == NAME, droplets)
    return droplets[0] if droplets else None


def find_image():
    images = requests.get(BASE_URL + '/images?private=true', headers=HEADERS).json()['images']
    images = filter(lambda d: d['name'] == NAME, images)
    return images[0] if images else None


def create():
    droplet = find_droplet()
    if droplet is not None:
        return
    image = find_image()
    if image is None:
        return
    requests.post(BASE_URL + '/droplets', headers=HEADERS, data=json.dumps({
        "image": image['id'],
        "name": NAME,
        "region": "nyc3",
        "size": "512mb"
    }))


def delete():
    droplet = find_droplet()
    if droplet is None:
        return
    requests.delete(BASE_URL + '/droplets/' + str(droplet['id']), headers=HEADERS)


if sys.argv[1] == 'ON':
    create()

if sys.argv[1] == 'OFF':
    delete()