/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com)
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

#ifndef __DEBR_HELP_EDIT_WIDGET__
#define __DEBR_HELP_EDIT_WIDGET__

#include <libgebr/gui/gui.h>
#include <libgebr/geoxml/geoxml.h>


G_BEGIN_DECLS


#define DEBR_TYPE_HELP_EDIT_WIDGET		(debr_help_edit_widget_get_type())
#define DEBR_HELP_EDIT_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), DEBR_TYPE_HELP_EDIT_WIDGET, DebrHelpEditWidget))
#define DEBR_HELP_EDIT_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), DEBR_TYPE_HELP_EDIT_WIDGET, DebrHelpEditWidgetClass))
#define DEBR_IS_HELP_EDIT_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), DEBR_TYPE_HELP_EDIT_WIDGET))
#define DEBR_IS_HELP_EDIT_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), DEBR_TYPE_HELP_EDIT_WIDGET))
#define DEBR_HELP_EDIT_WIDGET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), DEBR_TYPE_HELP_EDIT_WIDGET, DebrHelpEditWidgetClass))


typedef struct _DebrHelpEditWidget DebrHelpEditWidget;
typedef struct _DebrHelpEditWidgetClass DebrHelpEditWidgetClass;

struct _DebrHelpEditWidget {
	GebrGuiHelpEditWidget parent;
};

struct _DebrHelpEditWidgetClass {
	GebrGuiHelpEditWidgetClass parent_class;
};

GType debr_help_edit_widget_get_type(void) G_GNUC_CONST;

/**
 * debr_help_edit_widget_new:
 * @object: The #GebrGeoXmlDocument associated with this help edition.
 * @help: The initial help string to start editing.
 * @committed: If %TRUE the content is considered committed.
 *
 * Creates a new help edit widget for DéBR
 *
 * Returns: A new help edit widget for DéBR.
 */
GebrGuiHelpEditWidget * debr_help_edit_widget_new(GebrGeoXmlObject * object,
						  const gchar * help,
						  gboolean committed);

/**
 * debr_help_edit_widget_is_content_empty:
 * @self: a #DebrHelpEditWidget
 *
 * This widget limits the editable area to the content of the help, the index and header titles are auto generated. The
 * intent of this function is for checking if the editable area is empty, since checking for emptiness with the return
 * value of gebr_gui_help_edit_get_content will fail.
 *
 * Returns: %TRUE if the editable area is empty, %FALSE otherwise.
 */
gboolean debr_help_edit_widget_is_content_empty(DebrHelpEditWidget * self);

G_END_DECLS

#endif /* __DEBR_HELP_EDIT_WIDGET__ */
