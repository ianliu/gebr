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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

#include <libgebr/intl.h>
#include <libgebr/date.h>
#include <libgebr/gui/utils.h>

#include "validate.h"
#include "debr.h"
#include "callbacks.h"

/**
 * \internal
 */
struct validate {
	GtkWidget *widget;
	GtkWidget *text_view;

	GtkTextBuffer *text_buffer;
	GtkTreeIter iter;

	GebrGeoXmlFlow *menu;
	GtkTreeIter menu_iter;

	GebrGeoXmlValidate *geoxml_validate;
};

static void validate_free(struct validate *validate);
static gboolean validate_get_selected(GtkTreeIter * iter, gboolean warn_unselected);
static void validate_set_selected(GtkTreeIter * iter);
static void validate_clicked(void);

void validate_setup_ui(void)
{
	GtkWidget *hpanel;
	GtkWidget *scrolled_window;
	GtkWidget *frame;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	hpanel = gtk_hpaned_new();
	debr.ui_validate.widget = hpanel;

	/*
	 * Left side
	 */
	frame = gtk_frame_new("");
	gtk_paned_pack1(GTK_PANED(hpanel), frame, FALSE, FALSE);

	debr.ui_validate.list_store = gtk_list_store_new(VALIDATE_N_COLUMN,
							 GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
	debr.ui_validate.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(debr.ui_validate.list_store));
	gtk_container_add(GTK_CONTAINER(scrolled_window), debr.ui_validate.tree_view);
	gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 180, 30);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(debr.ui_validate.tree_view), FALSE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_validate.tree_view)),
				    GTK_SELECTION_MULTIPLE);
	g_signal_connect(GTK_OBJECT(debr.ui_validate.tree_view), "cursor-changed", G_CALLBACK(validate_clicked), NULL);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_validate.tree_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", VALIDATE_ICON);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_validate.tree_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", VALIDATE_FILENAME);

	/*
	 * Right side
	 */
	debr.ui_validate.text_view_vbox = gtk_vbox_new(FALSE, 0);
	gtk_paned_pack2(GTK_PANED(hpanel), debr.ui_validate.text_view_vbox, TRUE, TRUE);

	gtk_widget_show_all(debr.ui_validate.widget);

}

static void validate_append_text(struct validate *validate, const gchar * format, ...);
static void validate_append_text_emph(struct validate *validate, const gchar * format, ...);
static void validate_append_text_error(struct validate *validate, const gchar *program_path,
				       const gchar *parameter_path, const gchar * format, ...);
void validate_menu(GtkTreeIter * iter, GebrGeoXmlFlow * menu)
{
	struct validate *validate;
	GtkWidget *scrolled_window;
	GtkWidget *text_view;
	GtkTextBuffer *text_buffer;

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_box_pack_end(GTK_BOX(debr.ui_validate.text_view_vbox), scrolled_window, TRUE, TRUE, 0);
	text_buffer = gtk_text_buffer_new(NULL);
	text_view = gtk_text_view_new_with_buffer(text_buffer);
	gtk_widget_show(text_view);
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
	g_object_set(G_OBJECT(text_view), "editable", FALSE, "cursor-visible", FALSE, NULL);

	PangoFontDescription *font = pango_font_description_new();
	pango_font_description_set_family(font, "monospace");
	gtk_widget_modify_font(text_view, font);
	pango_font_description_free(font);

	GebrGeoXmlValidateOperations operations = (GebrGeoXmlValidateOperations) {
		.append_text = validate_append_text, 
		.append_text_emph = validate_append_text_emph, 
		.append_text_error = NULL, 
		.append_text_error_with_paths = validate_append_text_error, 
	};
	GebrGeoXmlValidateOptions options;
	options.all = TRUE;
	validate = g_new(struct validate, 1);
	*validate = (struct validate) {
		.widget = scrolled_window,
		.text_view = text_view,
		.text_buffer = text_buffer,
		.menu = menu,
		.menu_iter = *iter,
		.geoxml_validate = gebr_geoxml_validate_new(validate, operations, options)
	};
	gint error_count = gebr_geoxml_validate_report_menu(validate->geoxml_validate, menu);

	gtk_list_store_append(debr.ui_validate.list_store, &validate->iter);
	gtk_list_store_set(debr.ui_validate.list_store, &validate->iter,
			   VALIDATE_ICON, !error_count ? debr.pixmaps.stock_apply : debr.pixmaps.stock_cancel,
			   VALIDATE_FILENAME, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(menu)),
			   VALIDATE_POINTER, validate, -1);
	validate_set_selected(&validate->iter);
}

void validate_close(void)
{
	GtkTreeIter iter;
	struct validate *validate;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_validate.tree_view) {
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_validate.list_store), &iter, VALIDATE_POINTER, &validate, -1);
		validate_free(validate);
	}
}

void validate_clear(void)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(debr.ui_validate.list_store)) {
		struct validate *validate;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_validate.list_store), &iter, VALIDATE_POINTER, &validate, -1);
		validate_free(validate);
	}
}

/**
 * \internal
 * Frees \p validate its iter and interface.
 */
static void validate_free(struct validate *validate)
{
	gtk_list_store_remove(debr.ui_validate.list_store, &validate->iter);
	gtk_widget_destroy(validate->widget);
	gebr_geoxml_validate_free(validate->geoxml_validate);
	g_free(validate);
}

/**
 * \internal
 * Show selected menu report.
 */
static gboolean validate_get_selected(GtkTreeIter * iter, gboolean warn_unselected)
{
	if (gebr_gui_gtk_tree_view_get_selected(GTK_TREE_VIEW(debr.ui_validate.tree_view), iter) == FALSE) {
		if (warn_unselected)
			debr_message(GEBR_LOG_ERROR, _("No menu selected"));
		return FALSE;
	}

	return TRUE;
}

/**
 * \internal
 * Select \p iter.
 */
static void validate_set_selected(GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(debr.ui_validate.tree_view), iter);
}

/**
 * \internal
 * Show selected menu report.
 */
static void validate_clicked(void)
{
	GtkTreeIter iter;
	struct validate *validate;

	if (!validate_get_selected(&iter, FALSE))
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_validate.list_store), &iter, VALIDATE_POINTER, &validate, -1);

	gtk_container_forall(GTK_CONTAINER(debr.ui_validate.text_view_vbox), (GtkCallback) gtk_widget_hide, NULL);
	gtk_widget_show(validate->widget);
}

/**
 * \internal
 */
static void
validate_insert_text_valist(struct validate *validate, GtkTextTag * text_tag, GtkTextIter * iter, const gchar * format, va_list argp)
{
	if (format == NULL)
		return;

	gchar *string;
	string = g_strdup_vprintf(format, argp);
	gtk_text_buffer_insert_with_tags(validate->text_buffer, iter, string, -1, text_tag, NULL);
	g_free(string);
}

/**
 * \internal
 * Appends text to \p validate's text buffer applying \p text_tag to it.
 *
 * \param text_tag A GtkTextTag determining the style for \p format.
 * \param format Text to be inserted, in printf-like format.
 * \param argp List of arguments for \p format.
 *
 * \see validate_append_text
 */
static void
validate_append_text_valist(struct validate *validate, GtkTextTag * text_tag, const gchar * format, va_list argp)
{
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(validate->text_buffer, &iter);
	validate_insert_text_valist(validate, text_tag, &iter, format, argp);
}

/**
 * \internal
 * Appends \p format to \p validate's text buffer, applying \p text_tag to it.
 * 
 * \see validate_append_text_valist
 */
static void validate_append_text_with_tag(struct validate *validate, GtkTextTag * text_tag, const gchar * format, ...)
{
	va_list argp;

	va_start(argp, format);
	validate_append_text_valist(validate, text_tag, format, argp);
	va_end(argp);
}

/**
 * \internal
 */
static void validate_parse_link_click_callback(GtkTextView * text_view, GtkTextTag * link_tag, const gchar * url, struct validate *validate)
{
	GtkTreeIter menu_iter;

	if (!menu_get_selected(&menu_iter, FALSE) ||
	    !gebr_gui_gtk_tree_model_iter_equal_to(GTK_TREE_MODEL(debr.ui_menu.model), &menu_iter,
						   &validate->menu_iter))
		menu_select_iter(&validate->menu_iter);

	gchar *program_path = g_object_get_data(G_OBJECT(link_tag), "program_path_string");
	gchar *parameter_path = g_object_get_data(G_OBJECT(link_tag), "parameter_path_string");
	if (program_path != NULL) {
		menu_select_program_and_paramater(program_path, parameter_path);
		if (parameter_path != NULL)
			on_parameter_properties_activate();
		else
			on_program_properties_activate();	
	} else 
		on_menu_properties_activate();
}

/**
 * \internal
 * Appends text to \p validate's text buffer referencing \p url.
 */
static GtkTextTag *
validate_append_link(struct validate *validate, const gchar *text, const gchar *url) 
{
	GtkTextTag *tag;

	tag = gebr_gui_gtk_text_buffer_create_link_tag(validate->text_buffer, url, (GebrGuiGtkTextViewLinkClickCallback)validate_parse_link_click_callback, validate);
	validate_append_text_with_tag(validate, tag, text);

	return tag;
}

/**
 * \internal
 * Appends \p text into validate log with pair of style/values, as seen in #GtkTextTag properties.
 *
 * \see GtkTextTag
 */
static void
validate_append_text_with_property_list(struct validate *validate, const gchar * text,
					const gchar * first_property_name, ...)
{
	GtkTextTag *text_tag;
	va_list argp;

	text_tag = gtk_text_tag_new(NULL);
	gtk_text_tag_table_add(gtk_text_buffer_get_tag_table(validate->text_buffer), text_tag);
	va_start(argp, first_property_name);
	g_object_set_valist(G_OBJECT(text_tag), first_property_name, argp);
	va_end(argp);

	validate_append_text_with_tag(validate, text_tag, text);
}

/**
 * \internal
 * Appends emphasized text in validate log, with printf-like \p format.
 *
 * \see validate_append_text
 */
static void validate_append_text_emph(struct validate *validate, const gchar * format, ...)
{
	gchar *string;
	va_list argp;

	va_start(argp, format);
	string = g_strdup_vprintf(format, argp);
	validate_append_text_with_property_list(validate, string, "weight", PANGO_WEIGHT_BOLD, NULL);
	va_end(argp);
	g_free(string);
}

/**
 * \internal
 * Appends text in validate log, with printf-like \p format.
 */
static void validate_append_text(struct validate *validate, const gchar * format, ...)
{
	gchar *string;
	va_list argp;

	va_start(argp, format);
	string = g_strdup_vprintf(format, argp);
	validate_append_text_with_tag(validate, NULL, string);
	va_end(argp);
	g_free(string);
}

/**
 * \internal
 * Appends \p format into \p validate buffer, indicating error (by setting the text color to red).
 * If \p fix_flags in non-zero then add a fix link after text.
 * If \p edit_id is non-zero the add an edit link after text.
 */
static void validate_append_text_error(struct validate *validate, const gchar *program_path,
				       const gchar *parameter_path, const gchar * format, ...)
{
	gchar *string;
	va_list argp;
	va_start(argp, format);
	string = g_strdup_vprintf(format, argp);
	validate_append_text_with_property_list(validate, string, "foreground", "#ff0000", NULL);
	va_end(argp);
	g_free(string);

	GtkTextTag *link_tag;

	validate_append_text(validate, " ");
	link_tag = validate_append_link(validate, _("Edit"), "");

	gchar *tmp = g_strdup(program_path);
	g_object_set_data(G_OBJECT(link_tag), "program_path_string", tmp);
	gebr_gui_g_object_set_free_parent(link_tag, tmp);

	tmp = g_strdup(parameter_path);
	g_object_set_data(G_OBJECT(link_tag), "parameter_path_string", tmp);
	gebr_gui_g_object_set_free_parent(link_tag, tmp);
}

