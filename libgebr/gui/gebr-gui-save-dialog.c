/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or * (at your option) any later version.
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

#include <config.h>
#include <glib/gi18n-lib.h>

#include "gebr-gui-save-dialog.h"
#include "gebr-gui-utils.h"

#include <glib.h>
#include <glib/gprintf.h>

enum {
	OVERWRITE,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_EXTENSION
};

/*
 * Prototypes
 */

static void gebr_gui_save_dialog_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gebr_gui_save_dialog_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void gebr_gui_save_dialog_destroy(GtkObject *object);

//static guint save_dialog_signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(GebrGuiSaveDialog, gebr_gui_save_dialog, GTK_TYPE_FILE_CHOOSER_DIALOG);

/*
 * Private functions
 */

static void gebr_gui_save_dialog_class_init(GebrGuiSaveDialogClass * klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;

	gobject_class = G_OBJECT_CLASS(klass);
	object_class = GTK_OBJECT_CLASS(klass);
	gobject_class->set_property = gebr_gui_save_dialog_set_property;
	gobject_class->get_property = gebr_gui_save_dialog_get_property;
	object_class->destroy = gebr_gui_save_dialog_destroy;

	g_object_class_install_property(gobject_class,
					PROP_EXTENSION,
					g_param_spec_string("extension",
							    "Extension",
							    "Default extension to prepend to the file name.",
							    NULL,
							    (GParamFlags)(G_PARAM_READWRITE)));
}

static void gebr_gui_save_dialog_init(GebrGuiSaveDialog * self)
{
	self->extension = NULL;

	gtk_dialog_add_buttons(GTK_DIALOG(self),
			       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			       GTK_STOCK_SAVE, GTK_RESPONSE_OK,
			       NULL);
}

static void gebr_gui_save_dialog_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GebrGuiSaveDialog *dialog = GEBR_GUI_SAVE_DIALOG(object);

	switch (prop_id) {
	case PROP_EXTENSION:
		gebr_gui_save_dialog_set_default_extension(dialog, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_gui_save_dialog_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GebrGuiSaveDialog *dialog = GEBR_GUI_SAVE_DIALOG(object);

	switch (prop_id) {
	case PROP_EXTENSION:
		g_value_set_string(value, dialog->extension);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_gui_save_dialog_destroy(GtkObject *object)
{
	GebrGuiSaveDialog *dialog = GEBR_GUI_SAVE_DIALOG(object);
	if (dialog->extension) {
		g_free(dialog->extension);
		dialog->extension = NULL;
	}
}

/*
 * Public functions
 */

GtkWidget *gebr_gui_save_dialog_new(const gchar *title, GtkWindow *parent)
{
	GtkWidget *self;
       
	self = g_object_new(GEBR_GUI_TYPE_SAVE_DIALOG,
			    "title", title,
			    "action", GTK_FILE_CHOOSER_ACTION_SAVE,
			    NULL);
	((GebrGuiSaveDialog*)self)->extension = NULL;

	gtk_window_set_modal(GTK_WINDOW(self), TRUE);
	if (parent) {
		gtk_window_set_transient_for(GTK_WINDOW(self), parent);
		gtk_window_set_destroy_with_parent(GTK_WINDOW(self), TRUE);
	}

	return self;
}

void gebr_gui_save_dialog_set_default_extension(GebrGuiSaveDialog *self, const gchar *extension)
{
	g_return_if_fail(GEBR_GUI_IS_SAVE_DIALOG(self));

	if (self->extension)
		g_free(self->extension);
	self->extension = g_strdup(extension);
}

const gchar *gebr_gui_save_dialog_get_default_extension(GebrGuiSaveDialog *self)
{
	g_return_val_if_fail(GEBR_GUI_IS_SAVE_DIALOG(self), NULL);

	return self->extension;
}

gchar *gebr_gui_save_dialog_run(GebrGuiSaveDialog *self)
{
	GtkWidget *dialog;
	GString *filename;
	gboolean free_str = TRUE;

	g_return_val_if_fail(GEBR_GUI_IS_SAVE_DIALOG(self), NULL);

	filename = g_string_new(NULL);

	while (TRUE) {
		gchar *tmp;

		if (gtk_dialog_run(GTK_DIALOG(self)) != GTK_RESPONSE_OK)
			break;

		tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(self));
		g_string_assign(filename, tmp);
		g_free(tmp);

		if (self->extension && !g_str_has_suffix(filename->str, self->extension))
			g_string_append(filename, self->extension);

		if (g_file_test(filename->str, G_FILE_TEST_EXISTS)) {
			gchar *basename;
			basename = g_path_get_basename(filename->str);
			dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(self),
								    (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
								    GTK_MESSAGE_QUESTION,
								    GTK_BUTTONS_NONE,
								    _("<b>A file named \"%s\" already exists. "
								      "Do you want to replace it?</b>"), basename);
			gtk_dialog_add_buttons(GTK_DIALOG(dialog),
					       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					       _("_Replace"), GTK_RESPONSE_OK,
					       NULL);
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
								 _("The file you are saving already exists. "
								   "Replacing it will overwrite its contents."));

			g_free(basename);
			if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
				free_str = FALSE;
				break;
			}
			gtk_widget_destroy(dialog);
		} else {
			free_str = FALSE;
			break;
		}
	}

	gtk_widget_destroy(GTK_WIDGET(self));

	return g_string_free(filename, free_str);
}

