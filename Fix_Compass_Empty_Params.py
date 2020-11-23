#!/bin/python3

from lxml import etree
import sys
import os 

tree = etree.parse(sys.argv[1])
root = tree.getroot()

f = open('original.xml', 'wb')
f.write("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n".encode())
f.write(etree.tostring(root, pretty_print=False))
f.close()


print ("Test this output!")

#print (root.find('configuration').find('board').find('parameters').find('entry').find('key').text)

for child in root.findall('board'):
	for child2 in child.findall('channel'):
		for child3 in child2.findall('values'):
			for child4 in child3.findall('entry'):
				#print(child4.findall('key'))
				#print(child4.findall('value'))
				if(len(child4.findall('key')) and not len(child4.findall('value'))):
					print ('Key/value pair not found in node with key --->')
					for child5 in child4.findall('key'):
						print(child5.text)
					child4.getparent().remove(child4)


	'''
	if(element.tag):
		if('key' in element.tag and 'value' in element.tag):
			i=1			
			#print("%s - %s" % (element.tag, element.text))
		elif('key' in element.tag):
			i=1			
			#print("%s - %s" % (element.tag, element.text))
		if(element.tag=='entry' and ('key' in element.tag) and ('value' not in element.tag)):
			print("^Deleting this entry!")
			element.getparent().remove(element)
	'''

f = open('settings.xml', 'wb')
f.write("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n".encode())
f.write(etree.tostring(root, pretty_print=False))
f.close()

os.system("diff -y original.xml editedcopy.xml --suppress-common-lines")