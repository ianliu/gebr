/**
 * \file parameter.h
 */

/*   DeBR - GeBR Designer
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

#ifndef __PARAMETER_H
#define __PARAMETER_H

#include <gtk/gtk.h>
#include <libgebr/geoxml.h>

#include <libgebr/gui/parameter.h>

G_BEGIN_DECLS

/** Parameter type actions */
extern const GtkRadioActionEntry parameter_type_radio_actions_entries[];

/** Number of parameters types */
extern const gsize combo_type_map_size;

/**
 * Columns for parameters tree model.
 */
enum {
	PARAMETER_TYPE,		/**< A #GEBR_GEOXML_PARAMETER_TYPE. */
	PARAMETER_KEYWORD,	/**< The word used to construct the command line. */
	PARAMETER_LABEL,	/**< A descriptive label to be shown in GeBR interface. */
	PARAMETER_XMLPOINTER,	/**< The #GebrGeoXmlParameter pointer */
	PARAMETER_N_COLUMN
};

/**
 * Structure holding values for parameter's user interface.
 */
struct ui_parameter {
	GtkWidget *widget;

	GtkTreeStore *tree_store;
	GtkWidget *tree_view;
};

/**
 * Structure holding values for parameter edition.
 */
struct ui_parameter_dialog {
	GtkWidget *dialog;

	GebrGeoXmlParameter *parameter;

	GtkWidget *default_widget_hbox;
	GtkWidget *list_widget_hbox;

	/** Structure built with #gebr_gui_parameter_widget_new, to construct the parameters edition UI. */
	struct gebr_gui_parameter_widget *gebr_gui_parameter_widget;
	GtkWidget *separator_entry;
	GtkWidget *comma_separator;
	GtkWidget *space_separator;
	GtkWidget *other_separator;
	GtkWidget *fake_separator;
};

/**
 * Set interface and its callbacks.
 */
void parameter_setup_ui(void);

/**
 * Load current program parameters' to the UI.
 */
void parameter_load_program(void);

/**
 * Load selected parameter contents to its iter.
 */
void parameter_load_selected(void);

/**
 * Creates a new parameter.
 */
void parameter_new(void);

/**
 * Creates a new parameter with the specified \p type.
 */
void parameter_new_from_type(enum GEBR_GEOXML_PARAMETER_TYPE type);

/**
 * Removes the selected parameter; if \p confirm is TRUE, asks for confirmation.
 */
void parameter_remove(gboolean confirm);

/**
 * Moves the selected parameter to the top of the list.
 */
void parameter_top(void);

/**
 * Moves the selected parameter to the bottom of the list.
 */
void parameter_bottom(void);

/**
 * Open dialog to change type of current selected parameter.
 * \return TRUE if user accepted changes, FALSE otherwise.
 */
gboolean parameter_change_type_setup_ui(void);

/**
 * Cuts the selected parameter(s) to clipboard.
 */
void parameter_cut(void);

/**
 * Copies the selected parameter(s) to clipboard.
 */
void parameter_copy(void);

/**
 * Pastes the selected parameters from clipboard.
 */
void parameter_paste(void);

/**
 * Changes the type of the selected parameter to \p type.
 */
void parameter_change_type(enum GEBR_GEOXML_PARAMETER_TYPE type);

/**
 * Show parameter's properties according to its type.
 */
void parameter_properties(void);

/**
 * Get the selected parameter in the tree view, if any.
 *
 * \param iter Address of tree iterator to write to.
 * \param show_warning Whether to warn or not if no parameter is selected.
 * \return TRUE if there is a parameter selected and write it to \p iter; FALSE otherwise.
 */
gboolean parameter_get_selected(GtkTreeIter * iter, gboolean show_warning);

/**
 * Select \p iter loading its pointer.
 */
void parameter_select_iter(GtkTreeIter iter);

/**
 * Populates a \ref GtkMenu with the available types.
 * \param use_action Whether to use the action group or not.
 */
GtkWidget * parameter_create_menu_with_types(gboolean use_action);

G_END_DECLS

#endif				//__PARAMETER_H
