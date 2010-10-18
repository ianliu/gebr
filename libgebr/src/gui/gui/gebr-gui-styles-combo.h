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

/**
 * SECTION:gebr-gui-styles-combo
 * @title: GebrGuiStylesCombo Class
 * @short_description: A widget for choosing between styles
 */

#ifndef __GEBR_GUI_STYLES_COMBO_H__
#define __GEBR_GUI_STYLES_COMBO_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEBR_GUI_TYPE_STYLES_COMBO		(gebr_gui_styles_combo_get_type ())
#define GEBR_GUI_STYLES_COMBO(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_TYPE_STYLES_COMBO, GebrGuiStylesCombo))
#define GEBR_GUI_STYLES_COMBO_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_GUI_TYPE_STYLES_COMBO, GebrGuiStylesComboClass))
#define GEBR_GUI_IS_STYLES_COMBO(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_STYLES_COMBO))
#define GEBR_GUI_IS_STYLES_COMBO_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_GUI_STYLES_COMBO))
#define GEBR_GUI_STYLES_COMBO_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_GUI_STYLES_COMBO, GebrGuiStylesComboClass))

typedef struct _GebrGuiStylesCombo GebrGuiStylesCombo;
typedef struct _GebrGuiStylesComboClass GebrGuiStylesComboClass;
typedef struct _GebrGuiStylesComboPrivate GebrGuiStylesComboPrivate;

struct _GebrGuiStylesCombo {
	/*< private >*/
	GtkComboBox parent;

	GebrGuiStylesComboPrivate *priv;
};

struct _GebrGuiStylesComboClass {
	/*< private >*/
	GtkComboBoxClass parent_class;
};

/**
 * gebr_gui_styles_combo_new:
 * @path: the system path for fetching the styles.
 *
 * Creates a new #GebrGuiStylesCombo for choosing a styles sheet in folder given
 * by @path variable.
 */
GtkWidget *gebr_gui_styles_combo_new (const gchar *path);

/**
 * gebr_gui_styles_combo_get_selected:
 * @combo: a #GebrGuiStylesCombo
 *
 * Retrieves the path for the selected styles in @combo.
 *
 * Returns: a newly allocated string containing the path for the selected style.
 */
gchar *gebr_gui_styles_combo_get_selected (GebrGuiStylesCombo *self);

G_END_DECLS

#endif /* __GEBR_GUI_STYLES_COMBO_H__ */
