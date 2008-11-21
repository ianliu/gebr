/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2008  Br ulio Barros de Oliveira (brauliobo@gmail.com)
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
	static gchar **		files;
	static GOptionEntry	entries[] = {
		{ G_OPTION_REMAINING, 0, G_OPTION_FLAG_FILENAME, G_OPTION_ARG_FILENAME_ARRAY, &files, "",
			"arq1.flw arq2.mnu arq3.prj arq34.lne ..." },
		{NULL}
	};
	gint			ret;
	gint			i;
	GError *		error = NULL;
	GOptionContext *	context;

	context = g_option_context_new(NULL);
	g_option_context_set_summary(context,
		"LibGeoXml XML validator"
	);
	g_option_context_set_description(context,
		""
	);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_ignore_unknown_options(context, FALSE);
	/* Parse command line*/
	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE || argv == NULL) {
		fprintf(stderr, "%s: syntax error\n", argv[0]);
		fprintf(stderr, "Try %s --help\n", argv[0]);
		ret = -1;
		goto out;
	}

	for (i = 0; files[i] != NULL; ++i) {
		ret = geoxml_document_validate(files[i]);
		if (ret < 0) {
			printf("%s INVALID: ", files[i]);
			switch (ret) {
			case GEOXML_RETV_DTD_SPECIFIED:
				printf("DTD specified. The <DOCTYPE ...> must not appear in XML.\n"
					"libgeoxml will find the appriate DTD installed from version.\n");
				break;
			case GEOXML_RETV_INVALID_DOCUMENT:
				printf("Invalid document. The has a sintax error or doesn't match the DTD.\n"
					"In this case see the errors above\n");
				break;
			case GEOXML_RETV_CANT_ACCESS_FILE:
				printf("Can't access file. The file doesn't exist or there is not enough "
					"permission to read it.\n");
				break;
			case GEOXML_RETV_CANT_ACCESS_DTD:
				printf("Can't access dtd. The file's DTD couldn't not be found.\n"
					"It may be a newer version not support by this version of libgeoxml "
					"or there is an installation problem\n");
				break;
			case GEOXML_RETV_NO_MEMORY:
				printf("Not enough memory. "
					"The library stoped after an unsucessful memory allocation.\n");
				break;
			default:
				printf("Unspecified error %d.\n"
					"See library documentation at http://gebr.sf.net/libgeoxml/doc\n", ret);
				break;
			}
			continue;
		}

		printf("%s valid!\n", files[i]);
	}

	ret = 0;
	g_strfreev(files);
out:	g_option_context_free(context);

	return ret;
}
