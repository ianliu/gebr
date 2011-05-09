/*   libgebr - GêBR Library
 *   Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
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

#include <glib.h>
#include <glib/gstdio.h>

#include "../gebr-validator.h"

void test_gebr_validator_simple(void)
{
	GebrValidator *validator;
	GebrGeoXmlProject *proj;
	GebrGeoXmlParameter *param;
	gchar *validated = NULL;
	GError *error = NULL;

	proj = gebr_geoxml_project_new();
	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						      "pi", "3.14");

	validator = gebr_validator_new((GebrGeoXmlDocument**)&proj, NULL, NULL);

	gebr_validator_validate_param(validator, param, &validated, &error);
	g_assert_no_error(error);

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(proj));
	gebr_validator_free(validator);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/validator/simple", test_gebr_validator_simple);

	return g_test_run();
}


