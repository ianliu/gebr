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
	static GOptionEntry	entries[] = {
		{NULL}
	};
	gint			ret;
	gint			i;
	GError *		error = NULL;
	GOptionContext *	context;

	context = g_option_context_new("LibGeoXml XML validator");
	g_option_context_set_summary(context,
		""
	);
	g_option_context_set_description(context,
		""
	);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_ignore_unknown_options(context, FALSE);
	/*†Parse†command†line†*/
	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE || argv == NULL) {
		fprintf(stderr, "%s:†syntax†error\n", argv[0]);
		fprintf(stderr, "Try†%s†--≠≠help\n", argv[0]);
		ret = -1;
		goto out;
	}

	for (i = 1; argv[i] != NULL; ++i) {
		GeoXmlDocument *	document;

		ret = geoxml_document_load(&document, argv[i]);
		if (ret < 0) {
			fprintf(stderr, "Could not load file %s\n", argv[i]);
			continue;
		}
		ret = geoxml_document_save(document, argv[i]);
		if (ret < 0) {
			fprintf(stderr, "Could not save file %s\n", argv[i]);
			geoxml_document_free(document);
			continue;
		}
		printf("Upgraded file %s to version %s!\n", argv[i], geoxml_document_get_version(document));

		geoxml_document_free(document);
	}

	ret = 0;
out:	g_option_context_free(context);

	return ret;
}
