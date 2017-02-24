# -*- coding: utf-8 -*-

from startrek_client import Startrek
import pickle
import re
import random
import copy
import sys
import collections


print 'hello'
token = 'c6d3bb51b6fb4e5090aeae8eeb968872'
client = Startrek(useragent='', base_url='https://st-api.yandex-team.ru', token=token)


counter = 0
fields = collections.defaultdict(list)

mapscontent_names = [
    u'Неверное название',
    u'Неверное положение метки',
    u'Удалить с карты',
    u'Нет на карте',
    u'Неправильный маршрут пешком',
    u'Неправильное время маршрута',
    u'Я знаю маршрут лучше',
]


def make_summary(issue):
    # print issue.key
    # if issue.queue.name != 'MAPSCONTENT':
    if not issue.key.startswith('MAPSCONTENT'):
        return issue.summary
    return re.sub('\[\w+\]', '', issue.summary).strip()

def need_add_to_description(line):
    if 'Forwarded message' in line:
        return False
    if 'End of forwarded message' in line:
        return False
    if '<#' in line or '#>' in line:
        return False
    return True

class GeoTask(object):
    def __init__(self, issue):
        self.summary = make_summary(issue)
        # print self.summary
        self.created_at = issue.createdAt
        self.nmaps_link = ''
        self.permalink = ''
        self.link = ''
        self.x, self.y = None, None
        lines = []
        for line in issue.description.split('\n'):
            match = re.match('(\w+):(.*)', line)
            if not match:
                if need_add_to_description(line):
                    lines.append(line)
                continue
            field, rest = match.groups()
            fields[field].append(issue.key)
            if field == 'coords':
                self.x, self.y = map(float, rest.strip().split(','))
            elif field == 'nmaps_link':
                self.nmaps_link = rest.strip()
            elif field == 'permalink':
                self.permalink = rest.strip()
            elif field == 'link':
                self.link = rest.strip()
            else:
                lines.append(line)
        if self.x is None:
            print issue.key
            print issue.description
            print '=' * 100
        # print self.x, self.y
        self.st_link = 'https://st.yandex-team.ru/' + issue.key
        self.description = '\n'.join(lines)

    @property
    def text(self):
        res = ''
        res += '<p>{0}</p>'.format(self.created_at.encode('utf-8'))
        res += '<p><strong>{0}</strong></p>'.format(self.summary.encode('utf-8'))
        res += '<p><a href="{0}">{0}</a></p>'.format(self.st_link)
        if self.nmaps_link:
            res += '<p><a href="{0}">nmaps_link</a></p>'.format(self.nmaps_link.encode('utf-8'))
        if self.permalink:
            res += '<p><a href="{0}">permalink</a></p>'.format(self.permalink.encode('utf-8'))
        if self.link:
            res += '<p><a href="{0}">link</a></p>'.format(self.link.encode('utf-8'))
        res += '<p>{0}</p>'.format(self.description.encode('utf-8'))
        # FIXME 1st replacement is correct
        # return res.replace("'", "\\'")
        return res.replace("'", "")

    def make_random_copy(self):
        res = copy.copy(self)
        res.x = res.x + random.uniform(-2, 2)
        res.y = res.y + random.uniform(-2, 2)
        return res

# {0}
# link
# permalink
# coords



def geo_task_available(issue):
    if not hasattr(issue, 'description'):
        return False
    if issue.description is None:
        return False
    for line in issue.description.split('\n'):
        if line.startswith('coords:'):
            return True
    if issue.queue == 'MAPSCONTENT':
        name = re.sub('\[\w+\]', '', issue.summary).strip()
        if name not in mapscontent_names:
            return False

    return False


def handle_issue(issue):
    if not geo_task_available(issue):
        return

    global counter
    counter += 1

    # print issue.as_dict()

    # print issue.summary
    # print issue.key
    # print issue.description
    
    # for line in issue.description.split('\n'):
    #     match = re.match('(\w+):.*', line)
    #     if not match:
    #         continue
    #     global fields
    #     fields.add(match.groups()[0])

    return GeoTask(issue)

    # print '=' * 100


# for component in client.queues['MAPSCONTENT'].components:
#     print component, component.name, component.id


queues = {
    'MAPSCONTENT': [
        'belarus',
        'kazahstan',
        'Moscow',
        'spb',
        'SNG',
        'Ukraina',
        'russia',
    ],
    'MCU': [
        'callcenter-maps',
        'Ukraina',
    ]
}

# issues_list = []

# for queue, component_names in queues.iteritems():
#     component_ids = map(str, [component.id
#         for component in client.queues[queue].components
#         if component.name in component_names
#     ])

#     issues = client.issues.find(
#             filter={'queue': queue, 'created': {'from': '2016-01-01'}, 'components': component_ids},
#             # filter={'queue': queue, 'updated': {'from': '2016-01-01'}, 'components': component_ids},
#             per_page=100)
#     print queue, len(issues)

#     for index in range(1, issues.pages_count + 1):
#         page = issues.get_page(index)
#         for issue in page:
#             issues_list.append(issue)
# print len(issues_list)


# with open('issues.dump', 'wb') as f:
#     pickle.dump(issues_list, f)
# sys.exit(0)

def batching(iterable, batch_size):
    current = []
    for i in iterable:
        current.append(i)
        if len(current) >= batch_size:
            yield current
            current = []
    if current:
        yield current

# ll = [1, 2, 3, 4, 5, 6]
# for lp in batching(ll, 2):
#     print lp

# sys.exit(0)


with open('issues.dump', 'rb') as f:
    issues = pickle.load(f)



# for issue in issues:
#     handle_issue(issue)

geo_tasks = [handle_issue(issue) for issue in issues]
geo_tasks = filter(None, geo_tasks)


gt_to_insert = geo_tasks
# gt_to_insert = []
# for gt in geo_tasks:
#     for _ in range(1):
#         gt_to_insert.append(gt.make_random_copy())



with open('insert.sql', 'w') as f:
    for gt_batch in batching(gt_to_insert, 1000):
        f.write('insert into pkostikov.pins_from_startrack (position, comment) values \n')
        # f.write('insert into pins (position, comment) values \n')
        # f.write('insert into pins_100k (position, comment) values \n')
        # f.write('insert into pins_1kk (position, comment) values \n')
        values = ["(st_setsrid(st_geomfromtext('POINT (" + str(gt.x) + " " + str(gt.y) + ")'), 4326), '" + gt.text + "')" for gt in gt_batch]
        f.write(',\n'.join(values))
        f.write(';')

print 'issues_count', len(issues)
print 'counter', counter


known_fields = [
    'coords',
    'nmaps_link',
    'permalink',
    'link',
]

suspicious_fileds = [
    'email',
    'name',
    'browser',
    'ip',
    'region',
    'testBuckets',
    'yandexuid',
    'uid',
    'mailto',
    'From',
    'To',
    'Subject',
    'Date',
    'Sent',
    'https',
    'http',
    'status',
    'address',
    'wrong_coords',
    'correct_coords',
    'upd',
    'categories',
    'PS',
]
print 'fields:'
for field, keys in fields.iteritems():
    if field not in known_fields and field not in suspicious_fileds:
        print field, keys




