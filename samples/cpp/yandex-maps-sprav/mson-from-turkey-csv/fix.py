#!/usr/bin/python

import sys
import json

input_filename, output_filename = sys.argv[1:3]

print 'input_filename =', input_filename
print 'output_filename =', output_filename

correct_cards = []

with open(input_filename) as inf:
    cards = json.load(inf)
    for card in cards:
        if "addresses" not in card or not card["addresses"]:
            continue
        correct_cards.append(card)

with open(output_filename, 'w') as outf:
    ss = json.dumps(correct_cards, ensure_ascii=False, indent=4)
    outf.write(ss.encode('utf-8'))