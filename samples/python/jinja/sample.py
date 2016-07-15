# -*- coding: utf-8 -*-

from jinja2 import Template
template = Template(u'{{ corrections_count }} из этих правок сделан{% if corrections_count|int % 10 == 1 and corrections_count|int % 100 != 11 %}а{% else %}ы{% endif %} Вами')

for i in 1, 2, 3, 10, 11, 12, 13, 21, 22, 23, 25, 101, 211:
    print template.render(corrections_count=str(i))