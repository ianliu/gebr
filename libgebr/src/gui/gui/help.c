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
#else
#include <stdlib.h>
#endif

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>

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
typedef void (*set_help)(GebrGeoXmlObject * object, const gchar * help);
struct help_edit_data {
	WebKitWebView * web_view;
	JSContextRef context;
	GebrGeoXmlObject * object;
	set_help set_function;
	GString *html_path;
	GebrGuiHelpEdited edited_callback;
};

static GtkWidget *web_view_on_create_web_view(GtkDialog ** r_window);

/**
 * \internal
 * The CSS to highlight and editable area when the user pass
 * the cursor over it.
 */
#define css_editable_area_style \
	".content {"\
		"border: 2px solid Transparent;"\
		"padding: 3px;"\
	"}"\
	".content:hover {"\
		"border-color: black;"\
	"}"

/**
 * \internal
 * Main JS loaded after the page has been loaded.
 * Load CKEDITOR JS, CSS hover highlight, on click start/leave edition depending
 * on area clicked, index generation after edition, CKEDITOR load with configuration.
 */
static gchar * js_start_inline_editing = \
	"var editor = null;"
	"var editing_element = null;"
	"var editing_class = null;"
	"var CKEDITOR_BASEPATH='file://" CKEDITOR_DIR "/';"
	"var tag = document.createElement('script');"
	"tag.setAttribute('type', 'text/javascript');"
	"tag.setAttribute('src', 'file://" CKEDITOR_DIR "/ckeditor.js');"
	"document.getElementsByTagName('head')[0].appendChild(tag);"
	"function DestroyEditor(discard_changes) {"
		"if (editor) {"
			"editor.destroy(discard_changes);"
			"GenerateNavigationIndex(document);"
			"editor = null;"
			"editing_element = null;"
		"}"
	"}"
	"function GetEditableElements() {"
		"return [document.getElementsByClassName('content')[0]];"
	"}"
	"function InsertHoverCss() {"
		"var stl = document.createElement('style');"
		"stl.setAttribute('type', 'text/css');"
		"stl.appendChild(document.createTextNode('"css_editable_area_style"'));"
		"document.getElementsByTagName('head')[0].appendChild(stl);"
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
				"height:450,"
				"width: 390,"
				"resize_enabled:false,"
				"toolbarCanCollapse:false,"
				"toolbar:[['Source','Save', '-','Bold','Italic','Underline','-',"
					"'Subscript','Superscript','-','Undo','Redo','-',"
					"'NumberedList','BulletedList','Blockquote','Styles','-',"
					"'Link','Unlink','-','Find','Replace', '-' ]]});"
		"} else {" 
			"DestroyEditor();"
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
	"InsertHoverCss();"
	"document.body.addEventListener('click', on_click, false);"
	"";
#endif

/*
 * Public functions
 */

void gebr_gui_help_show(const gchar * uri, const gchar * title, const gchar * browser)
{
#ifdef WEBKIT_ENABLED
	GtkWidget *web_view;
	GtkDialog *dialog;

	web_view = web_view_on_create_web_view(&dialog);
	webkit_web_view_open(WEBKIT_WEB_VIEW(web_view), uri);
	gtk_window_set_title(GTK_WINDOW(dialog), title);
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
			       	GebrGuiHelpEdited edited_callback);

void gebr_gui_help_edit(GebrGeoXmlDocument * document, const gchar * editor, GebrGuiHelpEdited edited_callback)
{
	_gebr_gui_help_edit(gebr_geoxml_document_get_help(document), editor, GEBR_GEOXML_OBJECT(document),
			    (set_help)gebr_geoxml_document_set_help, edited_callback);
}

void gebr_gui_program_help_edit(GebrGeoXmlProgram * program, const gchar * editor, GebrGuiHelpEdited edited_callback)
{
	_gebr_gui_help_edit(gebr_geoxml_program_get_help(program), editor, GEBR_GEOXML_OBJECT(program),
			    (set_help)gebr_geoxml_program_set_help, edited_callback);
}

/*
 * Private functions
 */
#ifdef WEBKIT_ENABLED
/**
 * \internal
 * Save the updated HTML and call edited_callback if set.
 * If the editor is opened remove its HTML code.
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

	if (data->edited_callback)
		data->edited_callback(data->object, help->str);

	return help;
}

/**
 * \internal
 * Remove every ocurrence of \p data of #jscontext_to_data_hash
 */
static gboolean hash_foreach_remove(gpointer key, struct help_edit_data * value, struct help_edit_data * data)
{
	return (value == data) ? TRUE : FALSE;
}

/**
 * \internal
 * Update #jscontext_to_data_hash freeing memory alocated related to \p web_view
 */
static void web_view_on_destroy(WebKitWebView * web_view, struct help_edit_data * data)
{
	g_hash_table_foreach_remove(jscontext_to_data_hash, (GHRFunc)hash_foreach_remove, data);
	if (!g_hash_table_size(jscontext_to_data_hash)) {
		g_hash_table_unref(jscontext_to_data_hash);
		jscontext_to_data_hash = NULL;
	}
}

/**
 * \internal
 * Create the webview itself. Used for both viewing and editing HTML.
 * Returns the webview and the dialog created for it at \p r_window
 */
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

	/* Place the WebKitWebView in the GtkScrolledWindow */
	gtk_container_add(GTK_CONTAINER(scrolled_window), web_view);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), scrolled_window, TRUE, TRUE, 0);

	/* Show the result */
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	gtk_widget_show_all(window);

	return web_view;
}

/**
 * \internal
 * Treat the exit response of the dialog.
 */
static gboolean dialog_on_response(GtkDialog * dialog, gint response_id, struct help_edit_data * data)
{
	GString * help;

	help = help_edit_save(data->context, data);

	g_string_free(help, TRUE);
	g_unlink(data->html_path->str);
	g_string_free(data->html_path, TRUE);
	g_free(data);

	return TRUE;
}

/**
 * \internal
 * Change the title of the dialog according to the page title.
 */
static void web_view_on_title_changed(WebKitWebView * web_view, WebKitWebFrame * frame, gchar * title,
				      GtkWindow * window)
{
	gtk_window_set_title(window, title);
}

/**
 * \internal
 * CKEDITOR save toolbar button callback
 * Set at #web_view_on_load_finished
 */
JSValueRef js_callback_gebr_help_save(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
				      size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	struct help_edit_data * data;
		
	data = g_hash_table_lookup(jscontext_to_data_hash, function);
	help_edit_save(ctx, data);	

	return JSValueMakeUndefined(ctx);
}

/**
 * \internal
 * Load all page personalization for editing.
 */
static void web_view_on_load_finished(WebKitWebView * web_view, WebKitWebFrame * frame, struct help_edit_data * data)
{
	JSObjectRef function;
	gebr_js_evaluate(data->context, js_start_inline_editing);
	function = gebr_js_make_function(data->context, "gebr_help_save", js_callback_gebr_help_save);
	g_hash_table_insert(jscontext_to_data_hash, (gpointer)function, data);
	g_signal_handlers_disconnect_matched(G_OBJECT(web_view),
					     G_SIGNAL_MATCH_FUNC, 0, 0, NULL, G_CALLBACK(web_view_on_load_finished), data);
}

/**
 * \internal
 * Disable context menu for HTML editing.
 */
static gboolean web_view_on_button_press(GtkWidget * widget, GdkEventButton * event)
{
	if (event->button == 3)
		return TRUE;
	return FALSE;
}

/**
 * \internal
 * Esc cancel CKEDITOR edition
 */
static gboolean web_view_on_key_press(GtkWidget * widget, GdkEventKey * event, struct help_edit_data * data)
{
	if (event->keyval == GDK_Escape) {
		gebr_js_evaluate(data->context, "DestroyEditor(true);");
		return TRUE;
	}
	return FALSE;
}
#endif				//WEBKIT_ENABLED

/**
 * Load help into a temporary file and load with Webkit (if enabled).
 */
static void _gebr_gui_help_edit(const gchar *help, const gchar * editor,
			       	GebrGeoXmlObject * object, set_help set_function,
			       	GebrGuiHelpEdited edited_callback)
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
	struct help_edit_data *data;
	GtkWidget * web_view;
	GtkDialog * dialog;

	web_view = web_view_on_create_web_view(&dialog);
	data = g_new(struct help_edit_data, 1);
	data->object = object;
	data->set_function = set_function;
	data->html_path = html_path;
	data->edited_callback = edited_callback;
	data->web_view = WEBKIT_WEB_VIEW(web_view);
	data->context = webkit_web_frame_get_global_context(webkit_web_view_get_main_frame(data->web_view));

	g_signal_connect(dialog, "response", G_CALLBACK(dialog_on_response), data);
	g_signal_connect(web_view, "load-finished", G_CALLBACK(web_view_on_load_finished), data);
	g_signal_connect(web_view, "button-press-event", G_CALLBACK(web_view_on_button_press), data);
	g_signal_connect(web_view, "key-press-event", G_CALLBACK(web_view_on_key_press), data);
	g_signal_connect(web_view, "destroy", G_CALLBACK(web_view_on_destroy), data);
	g_signal_connect(web_view, "title-changed", G_CALLBACK(web_view_on_title_changed), dialog);
	g_hash_table_insert(jscontext_to_data_hash, (gpointer)data->context, data);
	webkit_web_view_open(WEBKIT_WEB_VIEW(data->web_view), html_path->str);
#else
	GString *cmd_line;
	gchar buffer[1024];
	GString *validated_help;

	/* initialization */
	cmd_line = g_string_new(NULL);
	validated_help = g_string_new(NULL);

	g_string_printf(cmd_line, "%s %s", editor, html_path->str);
	if (system(cmd_line->str))
		gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't open editor"),
					_("Cannot open your HTML editor.\n" "Please select it on 'Preferences'."));

	/* ensure UTF-8 encoding */
	if (g_utf8_validate(validated_help->str, -1, NULL) == FALSE) {
		gchar *converted;
		gsize bytes_read;
		gsize bytes_written;
		GError *error;

		error = NULL;
		converted = g_locale_to_utf8(validated_help->str, -1, &bytes_read, &bytes_written, &error);
		/* TODO: what else should be tried? */
		if (converted == NULL) {
			g_free(converted);
			gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't read edited file"),
						_("Could not read edited file.\n Please change the report encoding to UTF-8"));
			goto out;
		}

		g_string_assign(validated_help, converted);
		g_free(converted);
	}

	/* Read back the help from file */
	html_fp = fopen(html_path->str, "r");
	while (fgets(buffer, 1024, html_fp) != NULL)
		g_string_append(validated_help, buffer);
	fclose(html_fp);
	g_unlink(html_path->str);

	/* Finally, the edited help back to the document */
	set_function(object, validated_help->str);
	if (edited_callback != NULL)
		edited_callback(object, validated_help->str);

out:	g_string_free(cmd_line, TRUE);
	g_string_free(html_path, TRUE);
	g_string_free(validated_help, TRUE);
#endif
}
