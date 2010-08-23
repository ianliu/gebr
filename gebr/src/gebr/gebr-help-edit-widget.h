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

#ifndef __GEBR_HELP_EDIT_WIDGET__
#define __GEBR_HELP_EDIT_WIDGET__

#include <libgebr/gui.h>
#include <libgebr/geoxml.h>


G_BEGIN_DECLS


#define GEBR_TYPE_HELP_EDIT_WIDGET		(gebr_help_edit_widget_get_type())
#define GEBR_HELP_EDIT_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TYPE_HELP_EDIT_WIDGET, GebrHelpEditWidget))
#define GEBR_HELP_EDIT_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_TYPE_HELP_EDIT_WIDGET, GebrHelpEditWidgetClass))
#define GEBR_IS_HELP_EDIT_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TYPE_HELP_EDIT_WIDGET))
#define GEBR_IS_HELP_EDIT_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_TYPE_HELP_EDIT_WIDGET))
#define GEBR_HELP_EDIT_WIDGET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_TYPE_HELP_EDIT_WIDGET, GebrHelpEditWidgetClass))


typedef struct _GebrHelpEditWidget GebrHelpEditWidget;
typedef struct _GebrHelpEditWidgetClass GebrHelpEditWidgetClass;

struct _GebrHelpEditWidget {
	GebrGuiHelpEditWidget parent;
};

struct _GebrHelpEditWidgetClass {
	GebrGuiHelpEditWidgetClass parent_class;
};

GType gebr_help_edit_widget_get_type(void) G_GNUC_CONST;

/**
 * gebr_help_edit_widget_new:
 * Creates a new help edit widget for GêBR
 *
 * @document: The #GebrGeoXmlDocument associated with this help edition.
 * @help: The initial help string to start editing.
 *
 * Returns: A new help edit widget for GêBR.
 */
GtkWidget * gebr_help_edit_widget_new(GebrGeoXmlDocument * document, const gchar * help);

G_END_DECLS

#endif /* __GEBR_HELP_EDIT_WIDGET__ */
