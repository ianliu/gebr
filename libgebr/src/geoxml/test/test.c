/*   libgeoxml - An interface to describe seismic software in XML
 *   Copyright (C) 2007  Bráulio Barros de Oliveira (brauliobo@gmail.com)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>

#include <libgeoxml/libgeoxml.h>

#define print_test(str)						\
	do {							\
		printf("----------------------------------\n");	\
		printf("Testing %s\n", str);			\
		printf("----------------------------------\n"); \
	} while (0)

void
print_xml(GeoXmlDocument * document)
{
	char * xml;

	geoxml_document_to_string(document, &xml);
	printf("%s\n", xml);
	free(xml);
}

int
main (int argc, char **argv)
{
	GeoXmlFlow	* flow1;
	GeoXmlFlow	* flow2;
	GeoXmlFlow	* flow3;
	GeoXmlFlow	* flow4;
	GeoXmlFlow	* flow5;
	GeoXmlFlow	* flow6;
	GeoXmlDocument	* document;
	int		ret;
	
	/* flow_load */
	print_test("geoxml_document_load sufrac.mnu");
	if ((ret = geoxml_document_load(&document, "sufrac.mnu")) < 0) {
		switch (ret) {
			case GEOXML_RETV_DTD_SPECIFIED:
				printf("dtd specified\n");
				break;
			case GEOXML_RETV_INVALID_DOCUMENT:
				printf("invalid document\n");
				break;
			case GEOXML_RETV_CANT_ACCESS_FILE:
				printf("can't access file\n");
				break;
			case GEOXML_RETV_CANT_ACCESS_DTD:
				printf("can't access dtd\n");
				break;
			default:
				printf("unspecified error\n");
				break;
		}
		return 1;
	}
	flow1 = GEOXML_FLOW(document);
	print_xml(GEOXML_DOC(flow1));

	
	geoxml_flow_append_category(flow1, "abc");
	print_xml(GEOXML_DOC(flow1));

// 	print_test("geoxml_flow_load sugain.mnu");
// 	flow2 = geoxml_flow_load("sugain.mnu");
// 	print_xml(flow2);
//
// 	print_test("geoxml_flow_load suximage.mnu");
// 	flow3 = geoxml_flow_load("suximage.mnu");
// 	print_xml(flow3);
//
// 	/* flow_new */
// 	print_test("geoxml_flow_new");
// 	flow4 = geoxml_flow_new();
// 	print_xml(flow4);
//
// 	/* flow_clone */
// 	print_test("geoxml_flow_clone");
// 	flow5 = geoxml_flow_clone(flow1);
// 	print_xml(flow5);
//
// 	/* flow_save */
// 	print_test("geoxml_flow_save");
// 	geoxml_flow_save(flow1, "test.xml");

	/* flow_to_string: already tested in print_xml */

	/* error checking */
	/* TODO: */

	/* flow_get_program */


// 	geoxml_flow_free(flow1);
// 	geoxml_flow_free(flow2);
// 	geoxml_flow_free(flow3);
// 	geoxml_flow_free(flow4);
// 	geoxml_flow_free(flow5);

	return 0;
}
