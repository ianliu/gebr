/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include "pixmaps.h"

GdkPixbuf *
pixmaps_gebr_icon_16x16(void)
{
	static GdkPixbuf *	gebr_icon_16x16 = NULL;

	if (gebr_icon_16x16 == NULL) {
		GError * error;
		error = NULL;
		gebr_icon_16x16 = gdk_pixbuf_new_from_file(PIXMAPS_DIR "gebr-icon-16x16.png", &error);
	}

	return gebr_icon_16x16;
}

GdkPixbuf *
pixmaps_gebr_icon_32x32(void)
{
	static GdkPixbuf *	gebr_icon_32x32 = NULL;

	if (gebr_icon_32x32 == NULL) {
		GError * error;
		error = NULL;
		gebr_icon_32x32 = gdk_pixbuf_new_from_file(PIXMAPS_DIR "gebr-icon-32x32.png", &error);
	}

	return gebr_icon_32x32;
}

GdkPixbuf *
pixmaps_gebr_icon_64x64(void)
{
	static GdkPixbuf *	gebr_icon_64x64 = NULL;

	if (gebr_icon_64x64 == NULL) {
		GError * error;
		error = NULL;
		gebr_icon_64x64 = gdk_pixbuf_new_from_file(PIXMAPS_DIR "gebr-icon-64x64.png", &error);
	}

	return gebr_icon_64x64;
}

GdkPixbuf *
pixmaps_gebr_logo(void)
{
	static GdkPixbuf *	gebr_logo = NULL;

	if (gebr_logo == NULL) {
		GError * error;
		error = NULL;
		gebr_logo = gdk_pixbuf_new_from_file(PIXMAPS_DIR "gebr-logo.png", &error);
	}

	return gebr_logo;
}
