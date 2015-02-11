from lxml import etree


with open('sample.xml') as inf:
    xml_data = inf.read()


companies_xml = etree.fromstring(xml_data)
company_xml = companies_xml[0]
for phone_xml in company_xml.xpath('phone'):
    company_xml.remove(phone_xml)
    number_xmls = phone_xml.xpath('number')
    for number_xml in number_xmls:
        phone_xml.remove(number_xml)

    for number_xml in number_xmls:
        phone_copy_xml = etree.fromstring(etree.tostring(phone_xml))
        phone_copy_xml.append(number_xml)
        company_xml.append(phone_copy_xml)



with open('result.xml', 'w') as outf:
    outf.write(etree.tostring(company_xml))