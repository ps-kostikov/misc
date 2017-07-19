import json
import collections

with open('geocoder_formatted.json') as f:
    geocode_responses = json.load(f)

query_to_response = {}
for resp in geocode_responses:
    if 'query' not in resp or 'response' not in resp:
        continue
    query_to_response[resp['query']] = resp['response']

print (len(query_to_response))
kind_precision = collections.defaultdict(int)

with open('address_points_tasks.tsv') as f:
    lines = f.read().decode('utf-8').replace('\\\\', '\\').split('\n')

new_lines = []
for line in lines[1:]:
    try:
        id_, address, lat, lon = line.split('\t')
    except:
        continue
    kind, precision, geocoder_text = '', '', ''
    if address in query_to_response:
        response = query_to_response[address]
        kind = response['kind']
        precision = response['precision']
        geocoder_text = response['text']

    kind_precision["kind:{0} precision:{1}".format(kind, precision)] += 1

    new_lines.append('\t'.join([
        id_,
        address,
        lat,
        lon,
        kind,
        precision,
        geocoder_text
    ]))

new_lines = ['id\taddress\tlat\tlon\tkind\tprecision\tgeocoder_text'] + new_lines
with open('address_points_tasks_extended.tsv', 'w') as f:
    f.write('\n'.join(new_lines).encode('utf-8'))
