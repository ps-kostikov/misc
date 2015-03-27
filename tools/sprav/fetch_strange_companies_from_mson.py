#!/usr/bin/python

import sys
import json
import collections

input_filename, output_filename = sys.argv[1:3]

print 'input_filename =', input_filename
print 'output_filename =', output_filename

res = []

Id = collections.namedtuple("Id", "permalink sprav_id")

# ids_to_right_ids = {
#     "128275203": Id(1094694417, 42312293),
#     "16872": Id(1031073776, 13995177),
#     "141394": Id(1354567539, 103595782),
#     "4414": Id(1344259634, 102976407),
#     "2765213": Id(1111646543, 49862320),
#     "128313489": Id(1247935771, 92345603),
#     "106380": Id(1060588449, 27112197),
#     "128307934": Id(1092849501, 41481669),
#     "113001": Id(1056994300, 25500803),
#     "2210093": Id(1075551920, 33799785),
#     "102584": Id(1112601471, 106744372),
# }

ids_to_right_ids = {
    "117627": Id(0, 0),
    "272078": Id(0, 0),
    "127784690": Id(0, 0),
    "137863": Id(0, 0),
    "299493": Id(0, 0),
    "167009": Id(0, 0),
    "272078": Id(0, 0),
    "137863": Id(0, 0),
    "299493": Id(0, 0),
    "25773": Id(0, 0),
    "128317491": Id(0, 0),
    "140486": Id(0, 0),
}

# ids_to_right_ids = {
#     "4366771": Id(1001190339, 717273),
#     "128191170": Id(1073401172, 32829034),
#     "2765213": Id(1111646543, 49862320),
#     "33427": Id(1187389850, 75113820),
# }



with open(input_filename) as inf:
    companies = json.load(inf)
    for company in companies:
        export_id = company["origin"][0]["debug"]["id"]
        if export_id in ids_to_right_ids:
            id_ = ids_to_right_ids[export_id]
            company["debug"]["ammo_id"] = export_id

            if id_.sprav_id:
                company["debug"]["right_ids"] = [id_.sprav_id]
                company["debug"]["right_permalink"] = [id_.permalink]
            else:
                company["debug"]["right_ids"] = []
                company["debug"]["right_permalink"] = []
            res.append(company)


with open(output_filename, 'w') as outf:
    ss = json.dumps(res, ensure_ascii=False, indent=4)
    outf.write(ss.encode('utf-8'))
