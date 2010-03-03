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

#include <stdlib.h>

#include <webkit/webkit.h>
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

static GHashTable * jscontext_to_data_hash = NULL;
struct help_edit_data {
	WebKitWebView * web_view;
	JSContextRef context;

	GebrGeoXmlObject * object;
	GString *html_path;
	GebrGuiHelpEdited edited_callback;
	GebrGuiHelpRefresh refresh_callback;
	GtkActionGroup *actions;

	gboolean menu_edition;
};

static GtkWidget *web_view_on_create_web_view(GtkDialog ** r_window, struct help_edit_data * data);

static WebKitNavigationResponse
web_view_on_navigation_requested(WebKitWebView * web_view, WebKitWebFrame * frame,
				 WebKitNetworkRequest * request, struct help_edit_data * data);

static gboolean web_view_on_key_press(GtkWidget * widget, GdkEventKey * event, struct help_edit_data * data);

static void _gebr_gui_help_edit(const gchar *help, GebrGeoXmlObject * object, GebrGuiHelpEdited edited_callback,
				GebrGuiHelpRefresh refresh_callback, gboolean menu_edition);

static void on_help_edit_save_activate(GtkAction * action, struct help_edit_data * data);

static void on_help_edit_edit_toggled(GtkToggleAction * action, struct help_edit_data * data);

static void on_dialog_response(struct help_edit_data * data);

void on_help_edit_refresh_activate(GtkAction * action, struct help_edit_data * data);

/**
 * \internal
 * Main JS loaded after the page has been loaded.
 * Load CKEDITOR JS, CSS hover highlight, on click start/leave edition depending on area clicked, index generation after
 * edition, CKEDITOR load with configuration.
 */
static gchar * js_start_inline_editing = \
	"var document_clone = null;"
	"var editor = null;"
	"var editing_element = null;"
	"var CKEDITOR_BASEPATH='file://" CKEDITOR_DIR "/';"
	"var last_saved_content = null;"
	"function getHead(doc) {"
		"var head = doc.getElementsByTagName('head')[0];"
		"if (!head) {"
			"head = doc.createElement('head');"
			"doc.documentElement.insertBefore(head, doc.body);"
		"}"
		"return head;"
	"}"
	"function IsElementEditable(elt) {"
		"return (elt.nodeName.toLowerCase() == 'div'"
			"&& elt.className.indexOf('content') != -1);"
	"}"
	"function UpdateDocumentClone() {"
		"editor.updateElement();"
		"var content = GetEditableElements(document_clone)[0];"
		"while (content.firstChild)"
			"content.removeChild(content.firstChild);"
		"for (var i = GetEditableElements(document)[0].firstChild; i; i = i.nextSibling)"
			"content.appendChild(i.cloneNode(true));"
	"}"
	"function ToggleVisible(element) {"
		"var hidden = element.style.visibility == 'hidden';"
		"if (hidden) {"
			"element.style.visibility = 'visible';"
			"element.style.display = 'block';"
		"} else {"
			"element.style.visibility = 'hidden';"
			"element.style.display = 'none';"
		"}"
	"}"
	"function GetEditableElements(doc) {"
		"var content = doc.getElementsByClassName('content')[0];"
		"if (!menu_edition && !content) {"
			"content = doc.createElement('div');"
			"content.setAttribute('class', 'content');"
			"content.innerHTML = doc.body.innerHTML;"
			"doc.body.innerHTML = '';"
			"doc.body.appendChild(content);"
		"}"
		"return [content];"
	"}"
	"function UpgradeHelpFormat(doc) {"
		"var content = GetEditableElements(doc)[0];"
		"var links = content.getElementsByTagName('a');"
		"var blacklist = [];"
		"for (var i = 0; i < links.length; i++)"
			"if (links[i].innerHTML.search(/^\\s*$/) == 0)"
				"blacklist.push(links[i]);"
		"for (var i = 0; i < blacklist.length; i++)"
			"blacklist[i].parentNode.removeChild(blacklist[i]);"

		"GenerateNavigationIndex(doc);"
	"}"
	"function GenerateNavigationIndex(doc) {"
		"var navbar = doc.getElementsByClassName('navigation')[0];"
		"if (!navbar) return;"
		"var headers = GetEditableElements(doc)[0].getElementsByTagName('h2');"
		"navbar.innerHTML = '<h2>Index</h2><ul></ul>';"
		"var navlist = navbar.getElementsByTagName('ul')[0];"
		"for (var i = 0; i < headers.length; i++) {"
			"var anchor = 'header_' + i;"
			"var link = doc.createElement('a');"
			"link.setAttribute('href', '#' + anchor);"
			"link.appendChild(headers[i].cloneNode(true));"
			"var li = doc.createElement('li');"
			"li.appendChild(link);"
			"navlist.appendChild(li);"
			"headers[i].setAttribute('id', anchor);"
		"}"
	"}"
	"function OpenCkEditor(element) {"
		"if (editor) return;"
		"editing_element = element;"
		"editor = CKEDITOR.replace(element, {"
			"fullpage: true,"
			"height:300,"
			"width: menu_edition?390:'100%',"
			"resize_enabled:false,"
			"toolbarCanCollapse:false,"
			"toolbar:[['Source','-','Bold','Italic','Underline','-',"
				"'Subscript','Superscript','-','Undo','Redo','-','/',"
				"'NumberedList','BulletedList','Blockquote','Styles','-',"
				"'Link','Unlink','-','RemoveFormat','-','Find','Replace', '-' ]]});"
	"}"
	"function onCkEditorLoadFinished() {"
		"if (menu_edition) {"
			"UpgradeHelpFormat(document);"
			"UpgradeHelpFormat(document_clone);"
		"}"
		"OpenCkEditor(GetEditableElements(document)[0]);"
	"}"
	"function isContentSaved() {"
		"return editor.getData() == last_saved_content;"
	"}"
	"(function() {"
		"var head = getHead(document);"
		//force encode to UTF-8
		"var meta = head.getElementsByTagName('meta');"
		"var black_list = [];"
		"for (var i = 0; i < meta.length; i++) {"
			"var attr = meta[i].getAttribute('http-equiv');"
			"if (attr && attr.toLowerCase() == 'content-type')"
				"black_list.push(meta[i]);"
		"}"
		"for (var i = 0; i < black_list.length; i++)"
			"black_list[i].parentNode.removeChild(black_list[i]);"
		"meta = document.createElement('meta');"
		"meta.setAttribute('http-equiv', 'Content-Type');"
		"meta.setAttribute('content', 'text/html; charset=UTF-8');"
		"if (head.firstChild) {"
			"head.insertBefore(meta, head.firstChild);"
		"} else {"
			"head.appendChild(meta);"
		"}"
		//clone document for saving
		"document_clone = document.implementation.createDocument('', '', null);"
		"document_clone.appendChild(document.documentElement.cloneNode(true));"
		"last_saved_content = GetEditableElements(document)[0].innerHTML;"
		//append javascript to load
		"var tag = document.createElement('script');"
		"tag.setAttribute('type', 'text/javascript');"
		"tag.setAttribute('src', 'file://" CKEDITOR_DIR "/ckeditor.js');"
		"head.appendChild(tag);"
	"})();";

/*
 * Public functions
 */

void gebr_gui_help_show(const gchar * uri, const gchar * title)
{
	GtkWidget *web_view;
	GtkDialog *dialog;

	web_view = web_view_on_create_web_view(&dialog, NULL);
	webkit_web_view_open(WEBKIT_WEB_VIEW(web_view), uri);
	g_signal_connect(web_view, "navigation-requested", G_CALLBACK(web_view_on_navigation_requested), NULL);
	gtk_window_set_title(GTK_WINDOW(dialog), title);
}

void gebr_gui_help_edit(GebrGeoXmlDocument * document, GebrGuiHelpEdited edited_callback,
			GebrGuiHelpRefresh refresh_callback, gboolean menu_edition)
{
	_gebr_gui_help_edit(gebr_geoxml_document_get_help(document), GEBR_GEOXML_OBJECT(document),
			    edited_callback, refresh_callback, menu_edition);
}

void gebr_gui_program_help_edit(GebrGeoXmlProgram * program, GebrGuiHelpEdited edited_callback,
				GebrGuiHelpRefresh refresh_callback)
{
	_gebr_gui_help_edit(gebr_geoxml_program_get_help(program), GEBR_GEOXML_OBJECT(program),
			    edited_callback, refresh_callback, TRUE);
}

/*
 * Private functions
 */

/**
 * \internal
 * Save the updated HTML and call edited_callback if set.
 * If the editor is opened remove its HTML code.
 */
static void help_edit_save(struct help_edit_data * data)
{
	GString *help;
	GString *var_help;
	JSValueRef html;

	var_help = g_string_new(
			"(function(){"
				"last_saved_content = editor.getData();"
				"UpdateDocumentClone();"
				"if (menu_edition) {"
					"GenerateNavigationIndex(document);"
					"GenerateNavigationIndex(document_clone);"
				"}"
				"return document_clone.documentElement.outerHTML;"
			"})();");

	html = gebr_js_evaluate(data->context, var_help->str);
	help = gebr_js_value_get_string(data->context, html);

	if (gebr_geoxml_object_get_type(data->object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		gebr_geoxml_program_set_help(GEBR_GEOXML_PROGRAM(data->object), help->str);
	else
		gebr_geoxml_document_set_help(GEBR_GEOXML_DOCUMENT(data->object), help->str);

	if (data->edited_callback)
		data->edited_callback(data->object, help->str);

	g_string_free(help, TRUE);
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

static WebKitNavigationResponse web_view_on_navigation_requested(WebKitWebView * web_view, WebKitWebFrame * frame,
								 WebKitNetworkRequest * request, struct help_edit_data * data)
{
	const gchar * uri;
	uri = webkit_network_request_get_uri(request);
	if (g_str_has_prefix(uri, "file://") || g_str_has_prefix(uri, "about:"))
		return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;

#if GTK_CHECK_VERSION(2,14,0)
	if (!data) {
		GError *error = NULL;
		gtk_show_uri(NULL, uri, GDK_CURRENT_TIME, &error);
	}
#endif
	return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
}

static GtkActionEntry help_edit_actions[] = {
	{"save", GTK_STOCK_SAVE, NULL, NULL, N_("Save the current help"), G_CALLBACK(on_help_edit_save_activate)},
	{"refresh", GTK_STOCK_REFRESH, NULL, NULL, N_("Refresh the help content with automatic data fetched from the menu"), G_CALLBACK(on_help_edit_refresh_activate)},
};

static GtkToggleActionEntry help_edit_toggle_actions[] = {
	{"edit", GTK_STOCK_EDIT, NULL, NULL, N_("Toggles between read only and help edition"), G_CALLBACK(on_help_edit_edit_toggled), TRUE},
};

static const gchar * help_edit_ui_manager =
"<ui>"
	"<toolbar name='HelpToolBar'>"
		"<toolitem name='Save' action='save' />"
		"<toolitem name='Edit' action='edit' />"
		"<toolitem name='Refresh' action='refresh' />"
	"</toolbar>"
"</ui>";

/**
 * \internal
 * Create the webview itself.
 * Used for both viewing and editing HTML. Returns the webview and the dialog created for it at \p r_window.
 */
static GtkWidget *web_view_on_create_web_view(GtkDialog ** r_window, struct help_edit_data * data)
{
	static GtkWindowGroup *window_group = NULL;
	static GtkWidget *work_around_web_view = NULL;
	GtkWidget *window;
	GtkWidget *scrolled_window;
	GtkWidget *web_view;
	GtkWidget *content_area;
	GtkUIManager *ui;
	GtkActionGroup *actions;
	GError *error = NULL;

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

	/* Place the WebKitWebView in the GtkScrolledWindow */
	gtk_container_add(GTK_CONTAINER(scrolled_window), web_view);
#if GTK_CHECK_VERSION(2,14,0)
	content_area = gtk_dialog_get_content_area(GTK_DIALOG(window));
#else
	content_area = GTK_DIALOG(window)->vbox;
#endif
	if (data) {
		GtkAction *action;

		data->actions = actions = gtk_action_group_new("HelpEdit");
		gtk_action_group_add_actions(actions, help_edit_actions, G_N_ELEMENTS(help_edit_actions), data);
		gtk_action_group_add_toggle_actions(actions, help_edit_toggle_actions,
						    G_N_ELEMENTS(help_edit_toggle_actions), data);
		action = gtk_action_group_get_action(actions, "save");
		g_object_set(action, "is-important", TRUE, NULL);
		g_object_set(action, "short-label", _("Save"), NULL);
		action = gtk_action_group_get_action(actions, "edit");
		g_object_set(action, "is-important", TRUE, NULL);
		g_object_set(action, "short-label", _("Editing"), NULL);
		action = gtk_action_group_get_action(actions, "refresh");
		g_object_set(action, "is-important", TRUE, NULL);
		g_object_set(action, "short-label", _("Refresh"), NULL);

		ui = gtk_ui_manager_new();
		gtk_ui_manager_insert_action_group(ui, actions, 0);
		gtk_window_add_accel_group(GTK_WINDOW(window), gtk_ui_manager_get_accel_group(ui));
		if (!gtk_ui_manager_add_ui_from_string(ui, help_edit_ui_manager, -1, &error)) {
			g_message("Failed building gui: %s", error->message);
			g_error_free(error);
		}
		GtkWidget * toolbar = gtk_ui_manager_get_widget(ui, "/HelpToolBar");
		if (!data->refresh_callback) {
			GtkWidget * refresh = gtk_ui_manager_get_widget(ui, "/HelpToolBar/Refresh/");
			gtk_container_remove(GTK_CONTAINER(toolbar), refresh);
		}
		gtk_box_pack_start(GTK_BOX(content_area), toolbar, FALSE, FALSE, 0);
		g_object_unref(ui);
	}
	gtk_box_pack_start(GTK_BOX(content_area), scrolled_window, TRUE, TRUE, 0);

	/* Show the result */
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	gtk_widget_show_all(window);

	return web_view;
}

/**
 * \internal
 * Treat the exit response of the dialog.
 */
static void on_dialog_response(struct help_edit_data * data)
{
	gboolean is_content_saved;
	is_content_saved = JSValueToBoolean(data->context, gebr_js_evaluate(data->context, "isContentSaved();"));
	if (!is_content_saved) {
		gboolean response;
		response = gebr_gui_confirm_action_dialog(_("Save changes in help?"),
							  _("This help has unsaved changes. Do you want to save it?"));
		if (response)
			help_edit_save(data);
	}
	g_unlink(data->html_path->str);
	g_string_free(data->html_path, TRUE);
	g_object_unref(data->actions);
	g_free(data);
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
	help_edit_save(data);

	return JSValueMakeUndefined(ctx);
}

/**
 * \internal
 * Load all page personalization for editing.
 */
static void web_view_on_load_finished(WebKitWebView * web_view, WebKitWebFrame * frame, struct help_edit_data * data)
{
	JSObjectRef function;
	GString *var;

	data->web_view = web_view;
	data->context = webkit_web_frame_get_global_context(webkit_web_view_get_main_frame(data->web_view));

	var = g_string_new(NULL);
	g_string_printf(var, "var menu_edition = %s;", data->menu_edition ? "true" : "false");
	gebr_js_evaluate(data->context, var->str);
	g_string_free(var, TRUE);

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
 * Handle Escape key in WebView.
 */
static gboolean web_view_on_key_press(GtkWidget * widget, GdkEventKey * event, struct help_edit_data * data)
{
	if (event->keyval == GDK_Escape) {
		GtkWidget * dialog;
		dialog = gtk_widget_get_toplevel(widget);
		on_dialog_response(data);
		gtk_widget_destroy(dialog);
		return TRUE;
	}
	if (event->keyval == GDK_s && event->state & GDK_CONTROL_MASK) {
		help_edit_save(data);
		return TRUE;
	}
	return FALSE;
}

/**
 * Load help into a temporary file and load with Webkit (if enabled).
 */
static void _gebr_gui_help_edit(const gchar *help, GebrGeoXmlObject * object, GebrGuiHelpEdited edited_callback,
				GebrGuiHelpRefresh refresh_callback, gboolean menu_edition)
{
	FILE *html_fp;
	GString *html_path;

	if (!strlen(help))
		help = " ";

	/* write help to temporary file */
	html_path = gebr_make_temp_filename("XXXXXX.html");
	/* Write current help to temporary file */
	html_fp = fopen(html_path->str, "w");
	fputs(help, html_fp);
	fclose(html_fp);

	struct help_edit_data *data;
	GtkWidget * web_view;
	GtkDialog * dialog;

	data = g_new(struct help_edit_data, 1);
	data->object = object;
	data->html_path = html_path;
	data->edited_callback = edited_callback;
	data->refresh_callback = refresh_callback;
	data->menu_edition = menu_edition;

	web_view = web_view_on_create_web_view(&dialog, data);
	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		gtk_window_set_title(GTK_WINDOW(dialog), gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(object)));
	else
		gtk_window_set_title(GTK_WINDOW(dialog), gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(object)));

	g_signal_connect_swapped(dialog, "response", G_CALLBACK(on_dialog_response), data);
	g_signal_connect(web_view, "load-finished", G_CALLBACK(web_view_on_load_finished), data);
	g_signal_connect(web_view, "button-press-event", G_CALLBACK(web_view_on_button_press), data);
	g_signal_connect(web_view, "key-press-event", G_CALLBACK(web_view_on_key_press), data);
	g_signal_connect(web_view, "destroy", G_CALLBACK(web_view_on_destroy), data);
	g_signal_connect(web_view, "title-changed", G_CALLBACK(web_view_on_title_changed), dialog);
	g_signal_connect(web_view, "navigation-requested", G_CALLBACK(web_view_on_navigation_requested), data);
	g_hash_table_insert(jscontext_to_data_hash, (gpointer)data->context, data);
	webkit_web_view_open(WEBKIT_WEB_VIEW(web_view), html_path->str);
}

static void on_help_edit_save_activate(GtkAction * action, struct help_edit_data * data)
{
	help_edit_save(data);
}

static void on_help_edit_edit_toggled(GtkToggleAction * action, struct help_edit_data * data)
{
	const gchar * label;
	gboolean active;

	active = gtk_toggle_action_get_active(action);
	label = active? _("Editing"):_("Read only");

	g_object_set(action, "short-label", label, NULL);
	gtk_action_set_sensitive(gtk_action_group_get_action(data->actions, "save"), active);
	gtk_action_set_sensitive(gtk_action_group_get_action(data->actions, "refresh"), active);

	gebr_js_evaluate(data->context,
			 "(function(){"
			 	"var content = GetEditableElements(document)[0];"
			 	"var cke_editor = document.getElementById('cke_' + editor.name);"
				"editor.updateElement();"
				"GenerateNavigationIndex(document);"
				"ToggleVisible(content);"
				"ToggleVisible(cke_editor);"
			 "})();");
}

void on_help_edit_refresh_activate(GtkAction * action, struct help_edit_data * data)
{
	if (data->refresh_callback) {
		FILE * html_fp;
		GString * help;
		JSValueRef value;
		value = gebr_js_evaluate(data->context,
					 "(function(){"
					 	"UpdateDocumentClone();"
						"return document_clone.documentElement.outerHTML;"
					 "})();");
		help = gebr_js_value_get_string(data->context, value);
		(data->refresh_callback)(help, data->object);
		// 'help' is now refreshed, I hope ;)
		html_fp = fopen(data->html_path->str, "w");
		fputs(help->str, html_fp);
		fclose(html_fp);
		g_signal_connect(data->web_view, "load-finished", G_CALLBACK(web_view_on_load_finished), data);
		webkit_web_view_open(WEBKIT_WEB_VIEW(data->web_view), data->html_path->str);
	}
}
