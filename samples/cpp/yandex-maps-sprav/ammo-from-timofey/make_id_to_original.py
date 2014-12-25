#!/usr/bin/python


id_to_original = {}

with open('from_exl') as inf:
    lines = inf.readlines()
    for i in range(0, len(lines), 2):
        print lines[i]
        print lines[i + 1]
        id_, status = map(int, lines[i].strip().split())
        original_id = int(lines[i + 1].strip())
        if status != 1:
            continue
        id_to_original[id_] = original_id

with open('id_to_original', 'w') as outf:
    for id_, original_id in id_to_original.iteritems():
        print >>outf, id_, original_id
