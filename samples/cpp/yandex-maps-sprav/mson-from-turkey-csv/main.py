#!/usr/bin/python

import sys
import csv
import json

input_filename, output_filename = sys.argv[1:3]

print 'input_filename =', input_filename
print 'output_filename =', output_filename

cards = []

with open(input_filename) as csvfile:
    reader = csv.reader(csvfile)
    # skip header
    reader.next()

    for line in reader:
        id_, name, province, address, phone, url, lat, lon, rubric, source = line
        card = {
            "shipping_date": "20150323",
        }

        card["id"] = id_

        if source:
            card["vendor"] = source

        if address:
            if province:
                address_line = province + ", " + address
            else:
                address_line = address

            card["addresses"] = [
                {
                    "address_line": address_line
                }
            ]

        if lon and lat:
            lonv = float(lon.replace(',', '.'))
            while abs(lonv) > 90:
                lonv /= 10.
            latv = float(lat.replace(',', '.'))
            while abs(latv) > 90:
                latv /= 10.
            card["geometry"] = {
                "lon": lonv,
                "lat": latv
            }

        if phone:
            card["phones"] = [
                {
                    "phone": phone
                }
            ]

        if name:
            card["names"] = [
                {
                    "name": name
                }
            ]

        if url:
            card["urls"] = [
                {
                    "url": url 
                }
            ]

        cards.append(card)


with open(output_filename, 'w') as outf:
    ss = json.dumps(cards, ensure_ascii=False, indent=4)
    outf.write(ss)
    # outf.write(s.encode('utf-8'))