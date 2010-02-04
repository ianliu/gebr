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
#include <webkit/webkit.h>
#endif
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "../../intl.h"
#include "../../utils.h"

#include "help.h"
#include "utils.h"
#include "js.h"

/*
 * Declarations
 */

#ifdef WEBKIT_ENABLED
static GHashTable * jscontext_to_data_hash = NULL;
typedef void (*set_help)(GebrGeoXmlObject * object, const gchar help);
struct help_edit_data {
	WebKitWebView * web_view;
	JSContextRef context;
	GebrGeoXmlObject * object;
	set_help set_function;
	GString *html_path;
	GebrGuiHelpEditingFinished finish_callback;
};

static void web_view_on_title_changed(WebKitWebView * web_view, WebKitWebFrame * frame, gchar * title,
				      GtkWindow * window);
static GtkWidget *web_view_on_create_web_view(GtkDialog ** r_window);

static void web_view_on_load_finished(WebKitWebView * web_view, WebKitWebFrame * frame, struct help_edit_data * data);

static GtkWidget * web_view_create(struct help_edit_data * data);

static gchar * js_start_inline_editing = \
	"var editor = null;"
	"var editing_element = null;"
	"var editing_class = null;"
	"var CKEDITOR_BASEPATH='file://" CKEDITOR_DIR "/';"
	"var tag = document.createElement('script');"
	"tag.setAttribute('type', 'text/javascript');"
	"tag.setAttribute('src', 'file://" CKEDITOR_DIR "/ckeditor.js');"
	"document.getElementsByTagName('head')[0].appendChild(tag);"
	"function DestroyEditor() {"
		"if (editor) {"
			"editor.destroy();"
			"GenerateNavigationIndex(document);"
		"}"
	"}"
	"function GetEditableElements() {"
		"return [document.getElementsByClassName('content')[0]];"
	"}"
	"function IsElementEditable(elt) {"
		"return (elt.nodeName.toLowerCase() == 'div'"
			"&& elt.className.indexOf('content') != -1);"
	"}"
	"function on_click(evt) {"
		"var element = evt.target;"
		"var found_editable = false;"
		"while (element) {"
			"if (IsElementEditable(element)) {"
				"found_editable = true;"
				"break;"
			"} else if (editor && element.nodeName.toLowerCase() == 'span') {"
				"var id = element.getAttribute('id');"
				"if (id && id.indexOf('cke_editor') == 0)"
					"return;"
			"}"
			"element = element.parentNode;"
		"}"
		"if (found_editable) {"
			"DestroyEditor();" 
			"editing_element = element;"
			"editing_class = element.getAttribute('class');"
			"editor = CKEDITOR.replace(element, {"
				"fullpage: true,"
				"toolbar:[['Source','Save', '-','Bold','Italic','Underline','-',"
					"'Subscript','Superscript','-','Undo','Redo','-',"
					"'NumberedList','BulletedList','Blockquote','Format','-',"
					"'Link','Unlink','-','Find','Replace', '-' ]]});"
		"} else if (editor) {"
			"DestroyEditor();"
			"editor = null;"
			"editing_element = null;"
		"}"
	"}"
	"function GenerateNavigationIndex(doc) {"
		"var navbar = doc.getElementsByClassName('navigation')[0];"
		"var navlist = navbar.getElementsByTagName('ul')[0];"
		"var headers = GetEditableElements()[0].getElementsByTagName('h2');"
		"navlist.innerHTML = '';"
		"for (var i = 0; i < headers.length; i++) {"
			"var anchor = 'header_' + i;"
			"var link = doc.createElement('a');"
			"link.setAttribute('href', '#' + anchor);"
			"link.appendChild(doc.createTextNode(headers[i].innerHTML));"
			"var li = doc.createElement('li');"
			"li.appendChild(link);"
			"navlist.appendChild(li);"
			"headers[i].setAttribute('id', anchor);"
		"}"
	"}"
	"function UpgradeHelpFormat(doc) {"
		"var content = GetEditableElements()[0];"
		"var links = content.getElementsByTagName('a');"
		"var blacklist = [];"
		"for (var i = 0; i < links.length; i++)"
			"if (links[i].innerHTML.search(/^\\s*$/) == 0)"
				"blacklist.push(links[i]);"
		"for (var i = 0; i < blacklist.length; i++)"
			"blacklist[i].parentNode.removeChild(blacklist[i]);"

		"GenerateNavigationIndex(doc);"
	"}"
	"UpgradeHelpFormat(document);"
	"document.body.addEventListener('click', on_click, false);"
	"";
#endif

/*
 * Public functions
 */

void gebr_gui_help_show(const gchar * uri, const gchar * browser)
{
#ifdef WEBKIT_ENABLED
	GtkWidget *web_view;

	web_view = web_view_create(NULL);
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

static void _gebr_gui_help_edit(const gchar *help, const gchar * editor,
			       	GebrGeoXmlObject * object, set_help set_function,
			       	GebrGuiHelpEditingFinished finish_callback);

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

static GString *help_edit_save(JSContextRef context, struct help_edit_data * data)
{
	GString *help;
	JSValueRef html;

	const gchar * script_fetch_help =
		"(function() {"
			"var doc_clone = document.implementation.createDocument('', '', null);"
			"if (editor) editor.updateElement();"
			"doc_clone.appendChild(document.documentElement.cloneNode(true));"
			"var scripts = doc_clone.getElementsByTagName('script');"
			"for (var i = 0; i < scripts.length; i++)"
				"scripts[i].parentNode.removeChild(scripts[i]);"
			"if (editor) {"
				"var editing_element = doc_clone.getElementsByClassName(editing_class)[0];"
				"var ed = editing_element.nextSibling;"
				"editing_element.removeAttribute('style');"
				"ed.parentNode.removeChild(ed);"
			"}"
			"return doc_clone.documentElement.outerHTML;"
		"})();";

	html = gebr_js_evaluate(context, script_fetch_help);
	help = gebr_js_value_get_string(context, html);

	if (gebr_geoxml_object_get_type(data->object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		gebr_geoxml_program_set_help(GEBR_GEOXML_PROGRAM(data->object), help->str);
	else
		gebr_geoxml_document_set_help(GEBR_GEOXML_DOCUMENT(data->object), help->str);

	return help;
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
	data->web_view = WEBKIT_WEB_VIEW((web_view = web_view_create(data)));
	data->context = webkit_web_frame_get_global_context(webkit_web_view_get_main_frame(data->web_view));
	webkit_web_view_open(WEBKIT_WEB_VIEW(web_view), html_path->str);
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

#ifdef WEBKIT_ENABLED
static void web_view_main_frame_on_destroy(WebKitWebFrame * frame)
{
	JSContextRef context;

	context = webkit_web_frame_get_global_context(frame);
	g_hash_table_remove(jscontext_to_data_hash, context);
	if (!g_hash_table_size(jscontext_to_data_hash)) {
		g_hash_table_unref(jscontext_to_data_hash);
		jscontext_to_data_hash = NULL;
	}
}

static GtkWidget *web_view_on_create_web_view(GtkDialog ** r_window)
{
	static GtkWindowGroup *window_group = NULL;
	static GtkWidget *work_around_web_view = NULL;
	GtkWidget *window;
	GtkWidget *scrolled_window;
	GtkWidget *web_view;

	if (jscontext_to_data_hash == NULL)
		jscontext_to_data_hash = g_hash_table_new(NULL, NULL);
	if (window_group == NULL)
		window_group = gtk_window_group_new();
	if (!g_thread_supported())
		g_thread_init(NULL);
	/* WORKAROUND: Newer WebKitGtk versions crash on last WebKitWebView destroy,
	   so we always keep one instance of it. */
	if (work_around_web_view == NULL)
		work_around_web_view = webkit_web_view_new();

	window = gtk_dialog_new();
	if (r_window != NULL)
		*r_window = GTK_DIALOG(window);
	gtk_window_group_add_window(window_group, GTK_WINDOW(window));
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	web_view = webkit_web_view_new();
	if (g_signal_lookup("create-web-view", WEBKIT_TYPE_WEB_VIEW))
		g_signal_connect(web_view, "create-web-view", G_CALLBACK(web_view_on_create_web_view), NULL);
	g_signal_connect(web_view, "title-changed", G_CALLBACK(web_view_on_title_changed), window);
	g_signal_connect(webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(web_view)), "destroy",
			 G_CALLBACK(web_view_main_frame_on_destroy), NULL);

	/* Place the WebKitWebView in the GtkScrolledWindow */
	gtk_container_add(GTK_CONTAINER(scrolled_window), web_view);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), scrolled_window, TRUE, TRUE, 0);

	/* Show the result */
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	gtk_widget_show_all(window);

	return web_view;
}

static gboolean dialog_on_response(GtkDialog * dialog, gint response_id, struct help_edit_data * data)
{
	GString * help;

	help = help_edit_save(data->context, data);
	if (data->finish_callback)
		data->finish_callback(data->object, help->str);

	g_string_free(help, TRUE);
	g_unlink(data->html_path->str);
	g_string_free(data->html_path, TRUE);
	g_free(data);

	return TRUE;
}

static GtkWidget * web_view_create(struct help_edit_data * data)
{
	GtkWidget * web_view;
	GtkDialog * dialog;

	web_view = web_view_on_create_web_view(&dialog);
	if (data) {
		g_signal_connect(dialog, "response", G_CALLBACK(dialog_on_response), data);
		g_signal_connect(web_view, "load-finished", G_CALLBACK(web_view_on_load_finished), data);
		g_hash_table_insert(jscontext_to_data_hash, (gpointer)data->context, data);
	}
	return web_view;
}

static void web_view_on_title_changed(WebKitWebView * web_view, WebKitWebFrame * frame, gchar * title,
				      GtkWindow * window)
{
	gtk_window_set_title(window, title);
}

JSValueRef js_callback_gebr_help_save(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
				      size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	struct help_edit_data * data;
		
	data = g_hash_table_lookup(jscontext_to_data_hash, function);
	help_edit_save(ctx, data);	

	return JSValueMakeUndefined(ctx);
}

static void web_view_on_load_finished(WebKitWebView * web_view, WebKitWebFrame * frame, struct help_edit_data * data)
{
	JSObjectRef function;

	gebr_js_evaluate(data->context, js_start_inline_editing);
	function = gebr_js_make_function(data->context, "gebr_help_save", js_callback_gebr_help_save);
	g_hash_table_insert(jscontext_to_data_hash, (gpointer)function, data);

	g_signal_handlers_disconnect_matched(G_OBJECT(web_view),
					     G_SIGNAL_MATCH_FUNC, 0, 0, NULL, G_CALLBACK(web_view_on_load_finished), data);
}

#endif				//WEBKIT_ENABLED
