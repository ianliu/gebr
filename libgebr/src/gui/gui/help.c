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

#ifdef WEBKIT_ENABLED
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#endif
#include <gdk/gdk.h>
#include <glib/gstdio.h>

#include "../../intl.h"
#include "../../utils.h"

#include "help.h"
#include "utils.h"

/*
 * Declarations
 */

#ifdef WEBKIT_ENABLED
static void
libgebr_gui_help_show_on_title_changed(WebKitWebView * web_view,
				       WebKitWebFrame * frame, gchar * title, GtkWindow * window);
static GtkWidget *libgebr_gui_help_show_create_web_view(void);

static gchar * js_start_inline_editing =					\
	"var editor;"								\
	"document.body.addEventListener('dbclick', onDoubleClick, false);"	\
	"function onDoubleClick(evt) {"						\
	"	var element = ev.target || ev.srcElement;" 			\
	"	element = element.parentNode;"					\
	"	if (element.nodeName.toLowerCase() == 'div'"			\
	"		&& element.className.indexOf('editable') != -1)"	\
	"		replaceDiv(element);"					\
	"	else if (editor)"						\
	"		editor.destroy();"					\
	"}"									\
	"function replaceDiv(element) {"					\
	"	if (editor)"							\
	"		editor.destroy();"					\
	"	editor = CKEDITOR.replace(element);"				\
	"}";
#endif

/*
 * Public functions
 */

void gebr_gui_help_show(const gchar * uri, const gchar * browser)
{
#ifdef WEBKIT_ENABLED
	GtkWidget *web_view;

	web_view = libgebr_gui_help_show_create_web_view();
	webkit_web_view_open(WEBKIT_WEB_VIEW(web_view), uri);
#else
	GString *cmd_line;

	cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "%s %s &", browser, uri);
	if (system(cmd_line->str))
		gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't open browser"),
					_("Cannot open your web browser.\n" "Please select it on 'Preferences'."));

	g_string_free(cmd_line, TRUE);
#endif
}

typedef void (*set_help)(GebrGeoXmlObject * object, const gchar help);
struct help_edit_data {
	GebrGeoXmlObject * object;
	set_help set_function;
	GString *html_path;
	GebrGuiHelpEditingFinished finish_callback;
};

static void help_edit_on_web_view_destroy(WebKitWebView * web_view, struct help_edit_data * data)
{
	g_unlink(data->html_path->str);
	g_string_free(data->html_path, TRUE);
	g_free(data);
}

static void _gebr_gui_help_edit(const gchar *help, const gchar * editor,
			       	GebrGeoXmlObject * object, set_help set_function,
			       	GebrGuiHelpEditingFinished finish_callback)
{
	FILE *html_fp;
	GString *html_path;

	/* write help to temporary file */
	html_path = gebr_make_temp_filename("XXXXXX.html");
	/* Write current help to temporary file */
	html_fp = fopen(html_path->str, "w");
	fputs(help, html_fp);
	fclose(html_fp);
	
#ifdef WEBKIT_ENABLED
	GtkWidget *web_view;
	struct help_edit_data *data;

	data = g_new(struct help_edit_data, 1);
	data->object = object;
	data->set_function = set_function;
	data->html_path = html_path;
	data->finish_callback = finish_callback;

	web_view = libgebr_gui_help_show_create_web_view();
	webkit_web_view_open(WEBKIT_WEB_VIEW(web_view), html_path->str);
	g_signal_connect(web_view, "destroy", G_CALLBACK(help_edit_on_web_view_destroy), data);
#else
	GString *cmd_line;
	gchar buffer[BUFFER_SIZE];
	GString *help;

	/* initialization */
	cmd_line = g_string_new(NULL);
	help = g_string_new(NULL);

	g_string_printf(cmd_line, "%s %s", editor, uri);
	if (system(cmd_line->str))
		gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't open editor"),
					_("Cannot open your HTML editor.\n" "Please select it on 'Preferences'."));

	/* ensure UTF-8 encoding */
	if (g_utf8_validate(help->str, -1, NULL) == FALSE) {
		gchar *converted;
		gsize bytes_read;
		gsize bytes_written;
		GError *error;

		error = NULL;
		converted = g_locale_to_utf8(help->str, -1, &bytes_read, &bytes_written, &error);
		/* TODO: what else should be tried? */
		if (converted == NULL) {
			g_free(converted);
			gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't read edited file"),
						_("Could not read edited file.\n Please change the report encoding to UTF-8"));
			goto out;
		}

		g_string_assign(help, converted);
		g_free(converted);
	}

	/* Read back the help from file */
	html_fp = fopen(html_path->str, "r");
	while (fgets(buffer, BUFFER_SIZE, html_fp) != NULL)
		g_string_append(help, buffer);
	fclose(html_fp);
	g_unlink(html_path->str);

	/* Finally, the edited help back to the document */
	set_function(object, help->str);
	if (finish_callback != NULL)
		finish_callback(object, help->str);

out:	g_string_free(cmd_line, TRUE);
	g_string_free(html_path, TRUE);
	g_string_free(help, TRUE);
#endif
}

void gebr_gui_help_edit(GebrGeoXmlDocument * document, const gchar * editor, GebrGuiHelpEditingFinished finish_callback)
{
	_gebr_gui_help_edit(gebr_geoxml_document_get_help(document), editor, GEBR_GEOXML_OBJECT(document),
			    (set_help)gebr_geoxml_document_set_help, finish_callback);
}

void gebr_gui_program_help_edit(GebrGeoXmlProgram * program, const gchar * editor, GebrGuiHelpEditingFinished finish_callback)
{
	_gebr_gui_help_edit(gebr_geoxml_program_get_help(program), editor, GEBR_GEOXML_OBJECT(program),
			    (set_help)gebr_geoxml_program_set_help, finish_callback);
}

/*
 * Private functions
 */

#ifdef WEBKIT_ENABLED
static GtkWidget *libgebr_gui_help_show_create_web_view(void)
{
	static GtkWindowGroup *window_group = NULL;
	static GtkWidget *work_around_web_view = NULL;
	GtkWidget *window;
	GtkWidget *scrolled_window;
	GtkWidget *web_view;

	if (window_group == NULL)
		window_group = gtk_window_group_new();
	if (!g_thread_supported())
		g_thread_init(NULL);
	/* WORKAROUND: Newer WebKitGtk versions crash on last WebKitWebView destroy,
	   so we always keep one instance of it. */
	if (work_around_web_view == NULL)
		work_around_web_view = webkit_web_view_new();

	window = gtk_dialog_new();
	gtk_window_group_add_window(window_group, GTK_WINDOW(window));
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	web_view = webkit_web_view_new();
	if (g_signal_lookup("create-web-view", WEBKIT_TYPE_WEB_VIEW))
		g_signal_connect(web_view, "create-web-view", G_CALLBACK(libgebr_gui_help_show_create_web_view), NULL);
	g_signal_connect(web_view, "title-changed", G_CALLBACK(libgebr_gui_help_show_on_title_changed), window);

	/* Place the WebKitWebView in the GtkScrolledWindow */
	gtk_container_add(GTK_CONTAINER(scrolled_window), web_view);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), scrolled_window, TRUE, TRUE, 0);

	/* Show the result */
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	gtk_widget_show_all(window);

	return web_view;
}

static void
libgebr_gui_help_show_on_title_changed(WebKitWebView * web_view,
				       WebKitWebFrame * frame, gchar * title, GtkWindow * window)
{
	gtk_window_set_title(window, title);
}

#endif				//WEBKIT_ENABLED
