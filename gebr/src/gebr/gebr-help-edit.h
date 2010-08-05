/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#ifndef __GEBR_HELP_EDIT__
#define __GEBR_HELP_EDIT__

#include <libgebr/gui.h>
#include <libgebr/geoxml.h>


G_BEGIN_DECLS


#define GEBR_TYPE_HELP_EDIT		(gebr_help_edit_get_type())
#define GEBR_HELP_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TYPE_HELP_EDIT, GebrHelpEdit))
#define GEBR_HELP_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_TYPE_HELP_EDIT, GebrHelpEditClass))
#define GEBR_IS_HELP_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TYPE_HELP_EDIT))
#define GEBR_IS_HELP_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_TYPE_HELP_EDIT))
#define GEBR_HELP_EDIT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_TYPE_HELP_EDIT, GebrHelpEditClass))


typedef struct _GebrHelpEdit GebrHelpEdit;
typedef struct _GebrHelpEditClass GebrHelpEditClass;

struct _GebrHelpEdit {
	GebrGuiHelpEdit parent;
};

struct _GebrHelpEditClass {
	GebrGuiHelpEditClass parent_class;
};

GType gebr_help_edit_get_type(void) G_GNUC_CONST;

/**
 * gebr_help_edit_new:
 * Creates a new help edit widget for GêBR
 *
 * @flow: The #GebrGeoXmlFlow associated with this help edition.
 * @help: The initial help string to start editing.
 *
 * Returns: A new help edit widget for GêBR.
 */
GtkWidget * gebr_help_edit_new(GebrGeoXmlFlow * flow, const gchar * help);

G_END_DECLS

#endif /* __GEBR_HELP_EDIT__ */
