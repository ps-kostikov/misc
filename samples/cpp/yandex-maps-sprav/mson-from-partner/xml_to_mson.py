#!/usr/bin/python

from lxml import etree

# from yandex.maps.mining.modules.backa_partner_load import partner_convertor
import partner_convertor
from yandex.maps.mining.miningson import writer


xml_file_name = 'ASSAY-3454.xml'
with open(xml_file_name) as inf:
    xml_data = inf.read()

companies_xml = etree.fromstring(xml_data)


def patch_company_xml(company_xml):
    for phone_xml in company_xml.xpath('phone'):
        company_xml.remove(phone_xml)
        number_xmls = phone_xml.xpath('number')
        for number_xml in number_xmls:
            phone_xml.remove(number_xml)

        for number_xml in number_xmls:
            phone_copy_xml = etree.fromstring(etree.tostring(phone_xml))
            phone_copy_xml.append(number_xml)
            company_xml.append(phone_copy_xml)


with writer.Writer('out.json') as outf:
    for company_xml in companies_xml:
        patch_company_xml(company_xml)
        card = partner_convertor.to_miningson(company_xml, '20150211')
        outf.write(card)
