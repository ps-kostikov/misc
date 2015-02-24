#!/usr/bin/python

import sys
import jinja2

print 'hello'


class Identifier:
    def __init__(self, parts):
        self.parts = map(lambda s: s.lower(), parts);

    @property
    def snake_case(self):
        return '_'.join((s.lower() for s in self.parts))

    @property
    def pascal_case(self):
        return ''.join((s.capitalize() for s in self.parts))

    @property
    def camel_case(self):
        if not self.parts:
            return ''
        if len(self.parts) == 1:
            return self.parts[0].lower()
        return ''.join(
            [self.parts[0].lower()] +
            [s.capitalize() for s in self.parts[1:]]
        )


print sys.argv

enum_id = Identifier(sys.argv[1].split())
val_ids = [Identifier(s.split()) for s in sys.argv[2:]]

print enum_id.pascal_case
for v_id in val_ids:
    print v_id.snake_case

print ''

template = '''
enum class {{ enum_id.pascal_case }}
{
{%- for val_id in val_ids %}
    {{ val_id.pascal_case }}{% if not loop.last %}, {% endif %}
{%- endfor %}
};

std::ostream& operator<<(std::ostream& out, const {{ enum_id.pascal_case }}& {{ enum_id.camel_case }})
{
    switch ({{ enum_id.camel_case }}) {
{%- for val_id in val_ids %}
        case {{ enum_id.pascal_case }}::{{ val_id.pascal_case }}:
            return out << "{{ val_id.snake_case }}";
{%- endfor %}
    };
    return out;
}

std::istream& operator>>(std::istream& in, {{ enum_id.pascal_case }}& {{ enum_id.camel_case }})
{
    std::string str;
    in >> str;
{%- for val_id in val_ids %}
    {%- if loop.first %}
    if (str == "{{ val_id.snake_case }}") {
    {%- else %}
    } else if (str == "{{ val_id.snake_case }}") {
    {%- endif %}
        {{ enum_id.camel_case }} = {{ enum_id.pascal_case }}::{{ val_id.pascal_case }};
        return in;
    {%- if loop.last %}
    }
    {%- endif %}
{%- endfor %}
    throw maps::Exception() << "wrong value: " << str;
}


'''
jtemplate = jinja2.Template(template)
print jtemplate.render(enum_id=enum_id, val_ids=val_ids)


