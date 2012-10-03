/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but gebr.THOUT ANY gebr.RRANTY; without even the implied warranty
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "line.h"
#include "gebr.h"
#include "document.h"
#include "project.h"
#include "flow.h"
#include "callbacks.h"
#include "ui_project_line.h"
#include "ui_document.h"

static void
on_assistant_base_validate(GtkEntry *entry,
                           GtkAssistant *assistant)
{
	GtkWidget *current_page;
	gint page_number;
	const gchar *text;

	page_number = gtk_assistant_get_current_page(assistant);
	current_page = gtk_assistant_get_nth_page(assistant, page_number);
	text = gtk_entry_get_text(entry);

	gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_OPEN);
	if (text && *text) {
		if (text[0] == '/' || g_strrstr(text, "<HOME>") || g_strrstr(text, "$HOME")) {
			gtk_assistant_set_page_complete(assistant, current_page, TRUE);
			gtk_entry_set_icon_tooltip_markup(entry, GTK_ENTRY_ICON_SECONDARY, NULL);
		} else {
			gtk_assistant_set_page_complete(assistant, current_page, FALSE);
			gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIALOG_WARNING);
			gtk_entry_set_icon_tooltip_markup(entry, GTK_ENTRY_ICON_SECONDARY, _("You need to use an absolute path."));
		}
	} else {
		gtk_assistant_set_page_complete(assistant, current_page, FALSE);
		gtk_entry_set_icon_tooltip_markup(entry, GTK_ENTRY_ICON_SECONDARY, _("Choose a BASE path."));
	}
}

static void
on_assistant_import_validate(GtkEntry *entry,
                           GtkAssistant *assistant)
{
	GtkWidget *current_page;
	gint page_number;
	const gchar *text;

	page_number = gtk_assistant_get_current_page(assistant);
	current_page = gtk_assistant_get_nth_page(assistant, page_number);
	text = gtk_entry_get_text(entry);

	gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_OPEN);
	if (text && *text) {
		if (text[0] == '/' || g_strrstr(text, "<HOME>") || g_strrstr(text, "$HOME")) {
			gtk_assistant_set_page_complete(assistant, current_page, TRUE);
			gtk_entry_set_icon_tooltip_markup(entry, GTK_ENTRY_ICON_SECONDARY, NULL);
		} else {
			gtk_assistant_set_page_complete(assistant, current_page, FALSE);
			gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIALOG_WARNING);
			gtk_entry_set_icon_tooltip_markup(entry, GTK_ENTRY_ICON_SECONDARY, _("You need to use an absolute path."));
		}
	} else {
		gtk_assistant_set_page_complete(assistant, current_page, TRUE);
		gtk_entry_set_icon_tooltip_markup(entry, GTK_ENTRY_ICON_SECONDARY, NULL);
	}
}

void
on_properties_entry_changed(GtkEntry *entry,
			    GtkWidget *widget)
{
	const gchar *text;

	text = gtk_entry_get_text(entry);

	if (text && *text)
		gtk_widget_set_sensitive(widget, TRUE);
	else
		gtk_widget_set_sensitive(widget, FALSE);
}

static void
on_assistant_cancel(GtkWidget *widget)
{
	gtk_widget_destroy(widget);
	GtkTreeIter iter;
	if (project_line_get_selected(&iter, DontWarnUnselection))
		line_delete(&iter, FALSE);
}

typedef struct {
	gint progress_animation;
	gint timeout;
	GtkWidget *assistant;
	GtkBuilder *builder;
	gboolean title_ready;
	gboolean description_ready;
	gboolean email_ready;
} WizardData;

static void
on_assistant_entry_changed(WizardData *data)
{
	GtkWidget *current_page;
	current_page = gtk_assistant_get_nth_page(GTK_ASSISTANT(data->assistant), 0);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(data->assistant), current_page, (data->title_ready && data->email_ready));
}

static void
on_assistant_title_changed(GtkEntry *entry,
                           WizardData *data)
{
	const gchar *text = gtk_entry_get_text(entry);

	if (text && *text )
		data->title_ready = TRUE;
	else
		data->title_ready = FALSE;
	validate_entry(GTK_ENTRY(entry), !data->title_ready, _("Title cannot be empty"), _(""));
	on_assistant_entry_changed(data);
}

static void
on_assistant_description_changed(GtkEntry *entry,
                           WizardData *data)
{
	const gchar *text = gtk_entry_get_text(entry);

	if (text && *text )
		data->description_ready = TRUE;
	else
		data->description_ready = FALSE;
}

static void
on_assistant_email_changed(GtkEntry *entry,
                           WizardData *data)
{
	const gchar *text = gtk_entry_get_text(entry);
	data->email_ready = gebr_validate_check_is_email(text);
	validate_entry(GTK_ENTRY(entry), !data->email_ready, _("Invalid email"), _("Your email address"));
	on_assistant_entry_changed(data);
}

static gboolean
progress_bar_animate(gpointer user_data)
{
	WizardData *data = user_data;
	GObject *progress = gtk_builder_get_object(data->builder, "progressbar");
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progress));
	return TRUE;
}


static void
on_maestro_path_error(GebrMaestroServer *maestro,
		      gint error_id,
		      WizardData *data)
{
	g_source_remove(data->progress_animation);
	g_source_remove(data->timeout);

	GtkWidget *page2 = GTK_WIDGET(gtk_builder_get_object(data->builder, "main_progress"));
	GObject *container_progress = gtk_builder_get_object(data->builder, "container_progressbar");
	GObject *container_message = gtk_builder_get_object(data->builder, "container_message");
	GObject *image = gtk_builder_get_object(data->builder, "image_status");
	GObject *label = gtk_builder_get_object(data->builder, "label_status");

	gchar *summary_txt = NULL;
	GObject *label_summary = gtk_builder_get_object(data->builder, "label_summary");

	gtk_widget_hide(GTK_WIDGET(container_progress));
	gtk_widget_show_all(GTK_WIDGET(container_message));

	switch (error_id) {
	case GEBR_COMM_PROTOCOL_STATUS_PATH_OK:
		gtk_image_set_from_stock(GTK_IMAGE(image), GTK_STOCK_OK, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(GTK_LABEL(label), _("Success!"));
		gtk_assistant_set_page_type(GTK_ASSISTANT(data->assistant),
					    page2, GTK_ASSISTANT_PAGE_SUMMARY);
		gtk_assistant_set_page_title(GTK_ASSISTANT(data->assistant),
					     page2, _("Done"));
		gtk_assistant_set_page_complete(GTK_ASSISTANT(data->assistant), page2, TRUE);
		summary_txt = g_markup_printf_escaped("<span size='large'>%s</span>",
						      _("The directories were successfully created!"));
		break;
	case GEBR_COMM_PROTOCOL_STATUS_PATH_ERROR:
		gtk_image_set_from_stock(GTK_IMAGE(image), GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(GTK_LABEL(label), _("Press the back button to change the BASE directory\n"
						       "or the line title."));
		gtk_assistant_set_page_type(GTK_ASSISTANT(data->assistant),
					    page2, GTK_ASSISTANT_PAGE_CONFIRM);
		gtk_assistant_set_page_title(GTK_ASSISTANT(data->assistant),
					     page2, _("Error!"));
		gtk_assistant_set_page_complete(GTK_ASSISTANT(data->assistant), page2, FALSE);
		summary_txt = g_markup_printf_escaped("<span size='large'>%s</span>",
						      _("The directory could not be created!"));
		break;
	case GEBR_COMM_PROTOCOL_STATUS_PATH_EXISTS:
		gtk_image_set_from_stock(GTK_IMAGE(image), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(GTK_LABEL(label), _("You can change the BASE directory by pressing the\n"
						       "back button or use this folder anyway."));
		gtk_assistant_set_page_type(GTK_ASSISTANT(data->assistant),
					    page2, GTK_ASSISTANT_PAGE_CONFIRM);
		gtk_assistant_set_page_title(GTK_ASSISTANT(data->assistant),
					     page2, _("Warning!"));
		gtk_assistant_set_page_complete(GTK_ASSISTANT(data->assistant), page2, TRUE);
		summary_txt = g_markup_printf_escaped("<span size='large'>%s</span>",
						      _("The directory already exists!"));
		break;
	default:
		g_warn_if_reached();
		summary_txt = g_strdup("");
		break;
	}

	gtk_label_set_markup(GTK_LABEL(label_summary), summary_txt);
	g_free(summary_txt);

	g_signal_handlers_disconnect_by_func(maestro, on_maestro_path_error, data);

}

static gboolean
time_out_error(gpointer user_data)
{
	WizardData *data = user_data;

	g_source_remove(data->progress_animation);
	g_source_remove(data->timeout);

	GObject *image = gtk_builder_get_object(data->builder, "image_status");
	GObject *label = gtk_builder_get_object(data->builder, "label_status");
	GtkWidget *page2 = GTK_WIDGET(gtk_builder_get_object(data->builder, "main_progress"));
	GObject *progress = gtk_builder_get_object(data->builder, "container_progressbar");
	GObject *summary = gtk_builder_get_object(data->builder, "label_summary");
	GObject *container = gtk_builder_get_object(data->builder, "container_message");

	gtk_widget_hide(GTK_WIDGET(progress));
	gtk_widget_show(GTK_WIDGET(container));
	gtk_widget_show(GTK_WIDGET(image));
	gtk_widget_show(GTK_WIDGET(label));

	gchar *tmp = g_markup_printf_escaped("<span size='large'>%s</span>",
	                                     _("Timed Out!"));
	gtk_label_set_markup(GTK_LABEL(summary), tmp);
	g_free(tmp);
	gtk_image_set_from_stock(GTK_IMAGE(image), GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
	gtk_label_set_text(GTK_LABEL(label), _("Could not create directory.\nCheck if your maestro is connected."));

	gtk_assistant_set_page_type(GTK_ASSISTANT(data->assistant),
	                            page2, GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title(GTK_ASSISTANT(data->assistant),
	                             page2, _("Error!"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(data->assistant), page2, FALSE);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	g_signal_handlers_disconnect_by_func(maestro, on_maestro_path_error, data);

	return FALSE;
}

static void
on_assistant_close(GtkAssistant *assistant,
		   WizardData *data)
{
	gint page = gtk_assistant_get_current_page(assistant) + 1;

	if (page == 7) {
		GtkTreeIter iter;

		gebr_ui_document_set_properties_from_builder(GEBR_GEOXML_DOCUMENT(gebr.line), data->builder);
		project_line_get_selected(&iter, DontWarnUnselection);
		gtk_tree_store_set(gebr.ui_project_line->store, &iter,
				   PL_TITLE, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.line)), -1);
		project_line_info_update();
		gtk_widget_destroy(GTK_WIDGET(assistant));
	}
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	if (maestro) {
		gchar *home = g_build_filename(gebr_maestro_server_get_home_dir(maestro), NULL);
		gebr_geoxml_line_append_path(gebr.line, "HOME", home);
		g_free(home);
	}
}

static gchar *
resolve_home_variable_to_base(const gchar *base,
                              const gchar *home)
{
	gchar *used_base;
	if (g_str_has_prefix(base, "<HOME>")) {
		GString *buf = g_string_new(home);
		gchar **tmp = g_strsplit(base, "<HOME>", -1);

		g_string_append(buf, tmp[1]);

		used_base = g_string_free(buf, FALSE);

		g_strfreev(tmp);
		return used_base;
	} else if (g_str_has_prefix(base, "$HOME")) {
		GString *buf = g_string_new(home);
		gchar **tmp = g_strsplit(base, "$HOME", -1);

		g_string_append(buf, tmp[1]);
		used_base = g_string_free(buf, FALSE);
		g_strfreev(tmp);
		return used_base;
	} else
		return g_strdup(base);
}

static void
get_builder_objects(GtkBuilder *builder, const gchar *prefix, GHashTable *hash, const gchar **keys)
{

	gchar *aux;
	for (gint i = 0; keys[i]; i++) {
		aux = g_strdup_printf("%s%s", prefix, keys[i]);
		g_hash_table_insert(hash, (gpointer) g_strdup(keys[i]), gtk_builder_get_object(builder, aux));
		g_free(aux);
	}
}

static void
on_paths_button_clicked (GtkButton *button, gpointer pointer)
{
	const gchar *section = "projects_lines_line_paths";
	gchar *error;

	gebr_gui_help_button_clicked(section, &error);

	if (error) {
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE, error);
		g_free(error);
	}
}

static void
on_assistant_prepare(GtkAssistant *assistant,
		     GtkWidget *current_page,
		     WizardData *data)
{
	gint page = gtk_assistant_get_current_page(assistant) + 1;
	GObject *entry_base = gtk_builder_get_object(data->builder, "entry_base");

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	const gchar *maestro_addr = gebr_maestro_server_get_address(maestro);
	const gchar *home = gebr_maestro_server_get_home_dir(maestro);

	if (page == 1) {
		GObject *info_label= gtk_builder_get_object(data->builder, "info_label");
		gchar *info_label_text= g_markup_printf_escaped(_("The <i>Line</i> structure in GêBR gathers many "
                                                                  "processing flows.\n\nThe term <i>line</i> comes from "
                                                                  "the notion that the processing of a data "
                                                                  "set, acquired over a seismic line, is performed "
                                                                  "in several steps by the processing flows.\n\n"
                                                                  "This Line will be attached to the current Maestro "
                                                                  "(%s), which will be in charge of running its "
                                                                  "processing flows."), maestro_addr);
		gtk_label_set_markup(GTK_LABEL(info_label), info_label_text);
		g_free(info_label_text);
	}
	else if (page == 3) {
		GObject *entry_title = gtk_builder_get_object(data->builder, "entry_title");
		gchar *line_key = gebr_geoxml_line_create_key(gtk_entry_get_text(GTK_ENTRY(entry_title)));
		gchar *path = g_build_filename("<HOME>", "GeBR", line_key, NULL);

		const gchar *entr_text = gtk_entry_get_text(GTK_ENTRY(entry_base));
		if (!entr_text || !entr_text[0])
			gtk_entry_set_text(GTK_ENTRY(entry_base), path);

		GtkWidget *paths_help_button = GTK_WIDGET(gtk_builder_get_object(data->builder, "paths_help_button"));
		g_signal_connect(GTK_BUTTON(paths_help_button), "clicked", G_CALLBACK(on_paths_button_clicked), NULL);

		g_free(line_key);
		g_free(path);
	}
	else if (page == 5) {
		const gchar *base = gtk_entry_get_text(GTK_ENTRY(entry_base));

		GObject *label;
		gchar *text_maestro = g_markup_printf_escaped(_("Below is the hierarchy of directories GêBR is about to create.\n\n"
								"These directories will be created on the nodes of Maestro <b>%s</b>."), maestro_addr);
		label = gtk_builder_get_object(data->builder, "label_hierarchy_1");
		gtk_label_set_markup(GTK_LABEL(label), text_maestro);

		gchar *used_base = resolve_home_variable_to_base(base, home);

		gchar *text_base = g_markup_printf_escaped(_("<i>&lt;BASE&gt;: <b>%s</b></i>"), used_base);
		label = gtk_builder_get_object(data->builder, "label_hierarchy_2");
		gtk_label_set_markup(GTK_LABEL(label), text_base);

		g_free(text_maestro);
		g_free(text_base);
		g_free(used_base);
	}
	else if (page == 6) {
		GHashTable *labels = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
		GHashTable *entries = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL) ;

		const gchar *keys[] = {
			"title",
			"description",
			"author",
			"email",
			"base",
			"import",
			"maestro",
			NULL};

		get_builder_objects(data->builder, "label_rev_", labels, keys);
		get_builder_objects(data->builder, "entry_", entries, keys);

		for (gint i = 0; keys[i]; i++) {
			gchar *value;
			if (!g_strcmp0(keys[i], "maestro"))
				value = g_strdup(maestro_addr);
			else if (!g_strcmp0(keys[i], "base")) {
				const gchar *aux = gtk_entry_get_text(GTK_ENTRY(g_hash_table_lookup(entries, keys[i])));
				value = resolve_home_variable_to_base(aux, home);
			}
			else if(!g_strcmp0(keys[i], "import")) {
				const gchar *aux = gtk_entry_get_text(GTK_ENTRY(g_hash_table_lookup(entries, keys[i])));
				if (!*aux)
					value = g_strdup(_("<i>not defined</i>"));
				else
					value = resolve_home_variable_to_base(aux, home);
			}
			else {
				const gchar *aux = gtk_entry_get_text(GTK_ENTRY(g_hash_table_lookup(entries, keys[i])));
				if (!*aux)
					value = g_strdup(_("<i>not defined</i>"));
				else
					value = g_strdup(aux);
			}

			gtk_label_set_markup(GTK_LABEL(g_hash_table_lookup(labels, keys[i])), value);
			g_free(value);
			}
		g_hash_table_destroy(labels);
		g_hash_table_destroy(entries);
	}
	else if (page == 7) {
		GObject *container_progress = gtk_builder_get_object(data->builder, "container_progressbar");
		GObject *container_message = gtk_builder_get_object(data->builder, "container_message");
		GObject *label_summary = gtk_builder_get_object(data->builder, "label_summary");
		gchar *tmp = g_markup_printf_escaped("<span size='large'>%s</span>", _("Creating directories..."));
		gtk_label_set_markup(GTK_LABEL(label_summary), tmp);
		g_free(tmp);

		gtk_widget_hide(GTK_WIDGET(container_message));
		gtk_widget_show(GTK_WIDGET(container_progress));

		g_debug("Initiating progress");
		data->progress_animation = g_timeout_add(200, progress_bar_animate, data);
		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
		g_signal_connect(maestro, "path-error", G_CALLBACK(on_maestro_path_error), data);
		gebr_ui_document_send_paths_to_maestro(maestro, GEBR_COMM_PROTOCOL_PATH_CREATE,
						       NULL, gtk_entry_get_text(GTK_ENTRY(entry_base)));
		gtk_assistant_set_page_complete(GTK_ASSISTANT(data->assistant), current_page, FALSE);

		data->timeout = g_timeout_add(5000, time_out_error, data);
	}
}

static void
on_assistant_destroy(GtkWindow *window,
                     WizardData *data)
{
	g_free(data);
}

static void
line_setup_wizard(GebrGeoXmlLine *line)
{
	GtkBuilder *builder = gtk_builder_new();

	if (!gtk_builder_add_from_file(builder, GEBR_GLADE_DIR "/document-properties.glade", NULL))
		return;

	GtkWidget *page1 = GTK_WIDGET(gtk_builder_get_object(builder, "main_props"));
	GtkWidget *page2 = GTK_WIDGET(gtk_builder_get_object(builder, "paths_info"));
	GtkWidget *page3 = GTK_WIDGET(gtk_builder_get_object(builder, "vbox_base"));
	GtkWidget *page4 = GTK_WIDGET(gtk_builder_get_object(builder, "vbox_import"));
	GtkWidget *page5 = GTK_WIDGET(gtk_builder_get_object(builder, "vbox_hierarchy"));
	GtkWidget *page6 = GTK_WIDGET(gtk_builder_get_object(builder, "vbox_review"));
	GtkWidget *page7 = GTK_WIDGET(gtk_builder_get_object(builder, "main_progress"));
	GtkWidget *assistant = gtk_assistant_new();

	gtk_window_set_modal(GTK_WINDOW(assistant), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(assistant), GTK_WINDOW(gebr.window));
	gtk_window_set_position(GTK_WINDOW(assistant), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_title(GTK_WINDOW(assistant), _("Creating a new Line"));

	WizardData *data = g_new(WizardData, 1);
	data->assistant = assistant;
	data->builder = builder;
	data->title_ready = TRUE;
	data->description_ready = TRUE;
	data->email_ready = TRUE;
	g_signal_connect(assistant, "destroy", G_CALLBACK(on_assistant_destroy), data);
	g_signal_connect(assistant, "cancel", G_CALLBACK(on_assistant_cancel), NULL);
	g_signal_connect(assistant, "close", G_CALLBACK(on_assistant_close), data);
	g_signal_connect(assistant, "prepare", G_CALLBACK(on_assistant_prepare), data);

	gtk_assistant_append_page(GTK_ASSISTANT(assistant), page1);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page1, TRUE);
	gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page1, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page1, _("Line"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page1, FALSE);

	gtk_assistant_append_page(GTK_ASSISTANT(assistant), page2);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page2, TRUE);
	gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page2, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page2, _("Line Paths"));

	gtk_assistant_append_page(GTK_ASSISTANT(assistant), page3);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page3, TRUE);
	gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page3, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page3, _("BASE Path"));

	gtk_assistant_append_page(GTK_ASSISTANT(assistant), page4);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page4, TRUE);
	gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page4, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page4, _("IMPORT Path"));

	gtk_assistant_append_page(GTK_ASSISTANT(assistant), page5);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page5, TRUE);
	gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page5, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page5, _("Directories Hierarchy"));

	gtk_assistant_append_page(GTK_ASSISTANT(assistant), page6);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page6, TRUE);
	gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page6, GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page6, _("Summary"));

	gtk_assistant_append_page(GTK_ASSISTANT(assistant), page7);
	gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page7, GTK_ASSISTANT_PAGE_PROGRESS);
	gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page7, _("Creating directories..."));

	GObject *entry_title = gtk_builder_get_object(builder, "entry_title");
	GObject *entry_description = gtk_builder_get_object(builder, "entry_description");
	GObject *entry_base = gtk_builder_get_object(builder, "entry_base");
	GObject *entry_import = gtk_builder_get_object(builder, "entry_import");
	GObject *entry_author = gtk_builder_get_object(builder, "entry_author");
	GObject *entry_email = gtk_builder_get_object(builder, "entry_email");

	g_signal_connect(entry_base, "icon-press", G_CALLBACK(on_line_callback_base_entry_press), data->assistant);
	g_signal_connect(entry_import, "icon-press", G_CALLBACK(on_line_callback_import_entry_press), data->assistant);

	gchar *title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(line));
	gtk_entry_set_text(GTK_ENTRY(entry_title), title);
	g_free(title);

	gtk_entry_set_text(GTK_ENTRY(entry_author), gebr_geoxml_document_get_author(GEBR_GEOXML_DOCUMENT(gebr.project)));
	gtk_entry_set_text(GTK_ENTRY(entry_email), gebr_geoxml_document_get_email(GEBR_GEOXML_DOCUMENT(gebr.project)));

	on_assistant_entry_changed(data);
	on_assistant_title_changed(GTK_ENTRY(entry_title), data);
	on_assistant_description_changed(GTK_ENTRY(entry_description), data);
	on_assistant_email_changed(GTK_ENTRY(entry_email), data);
	g_signal_connect(entry_title, "changed", G_CALLBACK(on_assistant_title_changed), data);
	g_signal_connect(entry_description, "changed", G_CALLBACK(on_assistant_description_changed), data);
	g_signal_connect(entry_email, "changed", G_CALLBACK(on_assistant_email_changed), data);
	g_signal_connect(entry_base, "changed", G_CALLBACK(on_assistant_base_validate), assistant);
	g_signal_connect(entry_base, "focus-out-event", G_CALLBACK(on_line_callback_base_focus_out), NULL);
	g_signal_connect(entry_import, "changed", G_CALLBACK(on_assistant_import_validate), assistant);

	gtk_widget_show(assistant);
}

/* Public Methods */

gboolean
on_line_callback_base_focus_out(GtkWidget *widget,
                                GdkEvent  *event)
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	const gchar *simple_home = gebr_maestro_server_get_home_dir(maestro);
	gchar *mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));

	const gchar *text = gtk_entry_get_text(GTK_ENTRY(widget));

	gchar *relative = gebr_relativise_home_path(text, mount_point, simple_home);

	gtk_entry_set_text(GTK_ENTRY(widget), relative);

	g_free(mount_point);
	g_free(relative);
	return FALSE;
}

static void
create_base_import_file_chooser(gchar *title,
				GtkEntry *entry,
				GtkWindow *window)
{
	GtkWidget *file_chooser = gtk_file_chooser_dialog_new(title, window,
	                                                      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	                                                      GTK_STOCK_ADD, GTK_RESPONSE_OK,
	                                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), FALSE);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);

	gchar *prefix = gebr_maestro_server_get_sftp_prefix(maestro);
	const gchar *home = gebr_maestro_server_get_home_dir(maestro);
	gchar ***paths = gebr_generate_paths_with_home(home);

	gchar *new_text;
	const gchar *entry_text = gtk_entry_get_text(entry);
	gint response = gebr_file_chooser_set_remote_navigation(file_chooser,
	                                                        entry_text, prefix, paths, FALSE,
	                                                        &new_text);
	gchar *mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));

	if (response == GTK_RESPONSE_OK) {
		gchar *folder = gebr_relativise_home_path(new_text, mount_point, gebr_maestro_server_get_home_dir(maestro));

		gtk_entry_set_text(entry, gebr_remove_path_prefix(mount_point, folder));

		g_free(folder);
	}

	g_free(new_text);
	g_free(prefix);
	gebr_pairstrfreev(paths);
}

void
on_line_callback_base_entry_press(GtkEntry            *entry,
                                  GtkEntryIconPosition icon_pos,
                                  GdkEvent            *event,
                                  gpointer             user_data)
{
	GtkWindow *window = GTK_WINDOW(user_data);
	create_base_import_file_chooser("Choose BASE directory", entry, window);
}

void
on_line_callback_import_entry_press(GtkEntry            *entry,
                                    GtkEntryIconPosition icon_pos,
                                    GdkEvent            *event,
                                    gpointer             user_data)
{
	GtkWindow *window = GTK_WINDOW(user_data);
	create_base_import_file_chooser("Choose IMPORT directory", entry, window);
}

void line_new(void)
{
	GtkTreeIter iter;
	GtkTreeIter parent;
	GtkTreeModel *model;
	GebrGeoXmlLine *line;
	GebrGeoXmlDocument *doc;

	if (!project_line_get_selected (&parent, ProjectLineSelection))
		return;

	model = GTK_TREE_MODEL (gebr.ui_project_line->store);
	gtk_tree_model_get (model, &parent, PL_XMLPOINTER, &doc, -1);

	if (gebr_geoxml_document_get_type (doc) == GEBR_GEOXML_DOCUMENT_TYPE_LINE) {
		iter = parent;
		gtk_tree_model_iter_parent (model, &parent, &iter);
	}

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	line = GEBR_GEOXML_LINE(document_new(GEBR_GEOXML_DOCUMENT_TYPE_LINE));
	gebr_geoxml_document_set_title(GEBR_GEOXML_DOC(line), _("New Line"));
	gebr_geoxml_document_set_author(GEBR_GEOXML_DOC(line), gebr.config.username->str);
	gebr_geoxml_document_set_email(GEBR_GEOXML_DOC(line), gebr.config.email->str);

	if (maestro)
		gebr_geoxml_line_set_maestro(line, gebr_maestro_server_get_address(maestro));

	iter = project_append_line_iter(&parent, line);
	gebr_geoxml_project_append_line(gebr.project, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(line)));
	document_save(GEBR_GEOXML_DOC(gebr.project), TRUE, FALSE);
	document_save(GEBR_GEOXML_DOC(line), TRUE, FALSE);

	project_line_select_iter(&iter);

	line_setup_wizard(gebr.line);

	//document_properties_setup_ui(GEBR_GEOXML_DOCUMENT(gebr.line), on_properties_response, TRUE);
}

gboolean line_delete(GtkTreeIter * iter, gboolean warn_user)
{
	GebrGeoXmlLine * line;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), iter,
			   PL_XMLPOINTER, &line, -1);
	GtkTreeIter parent;
	gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &parent, iter);
	GebrGeoXmlProject * project;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &parent,
			   PL_XMLPOINTER, &project, -1);

	/* removes its flows */
	GebrGeoXmlSequence *line_flow;
	for (gebr_geoxml_line_get_flow(line, &line_flow, 0); line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
		const gchar *flow_source;

		flow_source = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		GString *path = document_get_path(flow_source);

		/* Remove flow's file */
		g_unlink(path->str);
		/* Remove flow's graph image */
		path = g_string_append(path, ".png");
		g_unlink(path->str);

		g_string_free(path, TRUE);

		/* log action */
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Deleting child Flow '%s'."), flow_source);
	}
	/* remove the line from its project */
	const gchar *line_filename = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(line));
	if (gebr_geoxml_project_remove_line(project, line_filename)) {
		document_save(GEBR_GEOXML_DOC(project), TRUE, FALSE);
		document_delete(line_filename);
	}
	/* GUI */
	gebr_remove_help_edit_window(GEBR_GEOXML_DOCUMENT(line));
	gtk_tree_store_remove(GTK_TREE_STORE(gebr.ui_project_line->store), iter);

	/* inform the user */
	if (warn_user) {
		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Deleting Line '%s'."),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(line)));
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Deleting Line '%s' from project '%s'."),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(line)),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(project)));
	}

	return TRUE;
}

void line_set_paths_to_relative(GebrGeoXmlLine *line, gboolean relative)
{
	GString *path = g_string_new(NULL);
	GebrGeoXmlSequence *line_path;
	gebr_geoxml_line_get_path(line, &line_path, 0);
	for (; line_path != NULL; gebr_geoxml_sequence_next(&line_path)) {
		g_string_assign(path, gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(line_path)));
		gebr_path_set_to(path, relative);
		gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(line_path), path->str);
	}
	g_string_free(path, TRUE);
}

void line_set_paths_to_empty (GebrGeoXmlLine *line)
{
	GebrGeoXmlSequence *seq;
	gebr_geoxml_line_get_path (line, &seq, 0);
	while (seq) {
		gebr_geoxml_sequence_remove (seq);
		gebr_geoxml_line_get_path (line, &seq, 0);
	}

	GebrGeoXmlSequence *line_flow;
	gebr_geoxml_line_get_flow(line, &line_flow, 0);
	for (; line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
		const gchar *filename = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		GebrGeoXmlFlow *flow;
	       	document_load((GebrGeoXmlDocument **)&flow, filename, TRUE);
		flow_set_paths_to_empty(flow);
	}
}

GtkTreeIter line_append_flow_iter(GebrGeoXmlFlow * flow, GebrGeoXmlLineFlow * line_flow)
{
	GtkTreeIter iter;

	/* add to the flow browser. */
	gtk_tree_store_append(gebr.ui_flow_browse->store, &iter, NULL);
	gebr_geoxml_object_ref(flow);
	gebr_geoxml_object_ref(line_flow);

	GebrUiFlow *ui_flow = gebr_ui_flow_new(flow, line_flow);
	GebrUiFlowBrowseType type = STRUCT_TYPE_FLOW;

	gtk_tree_store_set(gebr.ui_flow_browse->store, &iter,
			   FB_STRUCT_TYPE, type,
	                   FB_STRUCT, ui_flow,
			   -1);
	return iter;
}

void line_load_flows(void)
{
	GebrGeoXmlSequence *line_flow;
	GtkTreeIter iter;
	gboolean error = FALSE;

	flow_free();
	project_line_get_selected(&iter, DontWarnUnselection);

	/* iterate over its flows */
	gebr_geoxml_line_get_flow(gebr.line, &line_flow, 0);
	for (; line_flow; gebr_geoxml_sequence_next(&line_flow)) {
		GebrGeoXmlFlow *flow;

		const gchar *filename = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		int ret = document_load_with_parent((GebrGeoXmlDocument**)(&flow), filename, &iter, TRUE);
		if (ret) {
			error = TRUE;
			continue;
		}

		line_append_flow_iter(flow, GEBR_GEOXML_LINE_FLOW(line_flow));
	}

	flow_browse_revalidate_flows(gebr.ui_flow_browse);

	if (!error)
		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Flows loaded."));

	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter) == TRUE)
		flow_browse_select_iter(&iter);
}

void line_move_flow_top(void)
{
	GtkTreeIter iter;
	GebrGeoXmlSequence *line_flow;

	flow_browse_get_selected(&iter, FALSE);
	/* Update line XML */
	gebr_geoxml_line_get_flow(gebr.line, &line_flow,
				  gebr_gui_gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	gebr_geoxml_sequence_move_after(line_flow, NULL);
	document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, FALSE);
	/* GUI */
	gtk_tree_store_move_after(gebr.ui_flow_browse->store, &iter, NULL);
}

void line_move_flow_bottom(void)
{
	GtkTreeIter iter;
	GebrGeoXmlSequence *line_flow;

	flow_browse_get_selected(&iter, FALSE);
	/* Update line XML */
	gebr_geoxml_line_get_flow(gebr.line, &line_flow,
				  gebr_gui_gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	gebr_geoxml_sequence_move_before(line_flow, NULL);
	document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, FALSE);
	/* GUI */
	gtk_tree_store_move_before(gebr.ui_flow_browse->store, &iter, NULL);
}
