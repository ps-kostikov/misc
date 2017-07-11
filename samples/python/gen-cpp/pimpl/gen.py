import sys
import jinja2
import json

def defaultSetter(name):
    return 'set' + name[0].upper() + name[1:]

class Property:
    def __init__(self, dct):
        self.name = dct['name']
        self.type = dct['type']
        self.is_complex = dct['isComplex']
        self.setter = dct.get('setter')
        if self.setter == '_default':
            self.setter = defaultSetter(self.name)

class Description:
    def __init__(self, dct):
        self.name = dct['name']
        self.properties = map(Property, dct['properties'])

description_fn = sys.argv[1]
with open(description_fn) as f:
    description = Description(json.load(f))


template = '''

class {{ cls.name }}
{
public:
{%- for property in cls.properties %}
    {%- if property.is_complex %}
    const {{ property.type }}& {{ property.name }}() const;
    {%- else %}
    {{ property.type }} {{ property.name }}() const;
    {%- endif %}

{%- endfor %}

    COPYABLE_PIMPL_DECLARATIONS({{ cls.name }})
};

=========================================================

class {{ cls.name }}::Impl
{
public:
{%- for property in cls.properties %}
    {{ property.type }} {{ property.name }};
{%- endfor %}
};

{%- for property in cls.properties %}
    {%- if property.is_complex %}

const {{ property.type }}&
{{ cls.name }}::{{ property.name }}() const
{
    return impl_->{{ property.name }};
}
    {%- else %}

{{ property.type }}
{{ cls.name }}::{{ property.name }}() const
{
    return impl_->{{ property.name }};
}
    {%- endif %}

{%- endfor %}

COPYABLE_PIMPL_DEFINITIONS({{ cls.name }})

'''

jtemplate = jinja2.Template(template)
print jtemplate.render(cls=description)
