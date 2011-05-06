/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2011 GeBR core team (http://www.gebrproject.com/)
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

#ifndef __GEBR_VALIDATOR_H__
#define __GEBR_VALIDATOR_H__

#include <glib.h>
#include <geoxml/geoxml.h>

G_BEGIN_DECLS

typedef struct _GebrValidator GebrValidator;

GebrValidator *gebr_validator_new(GebrGeoXmlDocument *flow,
				  GebrGeoXmlDocument *line,
				  GebrGeoXmlDocument *proj);

gboolean gebr_validator_validate(GebrValidator       *self,
				 const gchar         *expression,
				 gchar              **validated,
				 GebrGeoXmlParameter *param,
				 GError             **err);

G_END_DECLS

#endif /* __GEBR_VALIDATOR_H__ */
