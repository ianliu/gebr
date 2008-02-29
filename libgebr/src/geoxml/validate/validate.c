/*   libgebr - GÍBR Library
 *   Copyright (C) 2007-2008  Br·ulio Barros de Oliveira (brauliobo@gmail.com)
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
#include "../geoxml.h"

int
main(int argc, char ** argv)
{
	int ret;

	if (argc != 2) {
		printf("Usage: geoxml_validate <filename>\n");
		return 1;
	}

	/* TODO: */
// 	printf(	"<program>  Copyright (C) <year>  <name of author>\n"
// 		"This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'."
// 		"This is free software, and you are welcome to redistribute it"
// 		"under certain conditions; type `show c' for details.");

	ret = geoxml_document_validate(argv[1]);
	if (ret < 0) {
		switch (ret) {
			case GEOXML_RETV_DTD_SPECIFIED:
				printf( "DTD specified. The <DOCTYPE ...> must not appear in XML.\n"
					"libgeoxml will find the appriate DTD installed from version.\n");
				break;
			case GEOXML_RETV_INVALID_DOCUMENT:
				printf(	"Invalid document. The has a sintax error or doesn't match the DTD.\n"
					"In this case see the errors above\n");
				break;
			case GEOXML_RETV_CANT_ACCESS_FILE:
				printf(	"Can't access file. The file doesn't exist or there is not enough "
					"permission to read it.\n");
				break;
			case GEOXML_RETV_CANT_ACCESS_DTD:
				printf( "Can't access dtd. The file's DTD couldn't not be found.\n"
					"It may be a newer version not support by this version of libgeoxml "
					"or there is an installation problem");
				break;
			case GEOXML_RETV_NO_MEMORY:
				printf("Not enough memory. The library stoped after an unsucessful memory allocation.\n");
				break;
			default:
				printf(	"Unspecified error %d.\n"
					"See library documentation at http://gebr.sf.net/libgeoxml/doc", ret);
				break;
		}
		return ret;
	}

	printf("Valid document (flow, line or project)!\n");

	return 0;
}
