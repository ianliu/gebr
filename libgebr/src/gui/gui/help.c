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
#include <regex.h>

#include <webkit/webkit.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>

#include "../../intl.h"
#include "../../utils.h"
#include "../../defines.h"

#include "help.h"
#include "utils.h"
#include "js.h"

/*
 * Declarations
 */

static GHashTable * jscontext_to_data_hash = NULL;
struct help_data {
	WebKitWebView * web_view;
	JSContextRef context;

	GebrGeoXmlObject * object;
	gboolean menu;
	GString *html_path;

	struct help_edit_data { 
		GebrGuiHelpEdited edited_callback;
		GebrGuiHelpRefresh refresh_callback;
		GtkActionGroup *actions;

		gboolean menu_refresh;
	} * edit;
};

static GtkWidget *web_view_on_create_web_view(GtkDialog ** r_window, struct help_data * data);

static WebKitNavigationResponse
web_view_on_navigation_requested(WebKitWebView * web_view, WebKitWebFrame * frame,
				 WebKitNetworkRequest * request, struct help_data * data);

static gboolean web_view_on_key_press(GtkWidget * widget, GdkEventKey * event, struct help_data * data);

static void generate_menu_links_index(struct help_data * data);
static void __gebr_gui_help_save_custom_help_to_file(struct help_data * data, const gchar *help);
static void __gebr_gui_help_save_help_to_file(struct help_data * data);

static GtkWidget* _gebr_gui_help_edit(GebrGeoXmlObject * object, GebrGuiHelpEdited edited_callback,
				      GebrGuiHelpRefresh refresh_callback, gboolean menu_edition);

static void on_help_edit_save_activate(GtkAction * action, struct help_data * data);

static void on_help_edit_edit_toggled(GtkToggleAction * action, struct help_data * data);

static void on_dialog_response(struct help_data * data);

static void on_help_edit_refresh_activate(GtkAction * action, struct help_data * data);

static void web_view_on_load_finished(WebKitWebView * web_view, WebKitWebFrame * frame, struct help_data * data);

/**
 * \internal
 * Main JS loaded after the page has been loaded.
 * Load CKEDITOR JS, CSS hover highlight, on click start/leave edition depending on area clicked, index generation after
 * edition, CKEDITOR load with configuration.
 */
static gchar * start_js = \
	"function getHead() {"
		"var head = document.getElementsByTagName('head')[0];"
		"if (!head) {"
			"head = document.createElement('head');"
			"document.documentElement.insertBefore(head, document.body);"
		"}"
		"return head;"
	"}"
	"function forceUtf8() {"
		"var head = getHead();"
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
	"}"
	"function addScript(source) {"
		"var script = document.createElement('script');"
		"script.type = 'text/javascript';"
		"script.id = source;"
		"script.src = 'file://" CKEDITOR_DIR "/' + source + '.js';"
		"script.defer = false;"
		"getHead().appendChild(script);"
	"}"
	"forceUtf8();"
	"var document_clone = document.implementation.createDocument('', '', null);"
	"document_clone.appendChild(document.documentElement.cloneNode(true));"
	"addScript('help');"
	"";

/*
 * Public functions
 */

void gebr_gui_help_show(GebrGeoXmlObject * object, gboolean menu, const gchar *help, const gchar * title)
{
	GtkWidget *web_view;
	GtkDialog *dialog;
	struct help_data *data;

	data = g_new(struct help_data, 1);
	data->object = object;
	data->menu = menu;
	data->html_path = g_string_new("");
	data->edit = NULL;
	__gebr_gui_help_save_custom_help_to_file(data, help);

	web_view = web_view_on_create_web_view(&dialog, NULL);
	webkit_web_view_open(WEBKIT_WEB_VIEW(web_view), data->html_path->str);
	g_signal_connect_swapped(dialog, "response", G_CALLBACK(on_dialog_response), data);
	g_signal_connect(web_view, "navigation-requested", G_CALLBACK(web_view_on_navigation_requested), data);
	g_signal_connect(web_view, "load-finished", G_CALLBACK(web_view_on_load_finished), data);
	gtk_window_set_title(GTK_WINDOW(dialog), title);
}

GtkWidget* gebr_gui_help_edit(GebrGeoXmlDocument * document, GebrGuiHelpEdited edited_callback,
			      GebrGuiHelpRefresh refresh_callback, gboolean menu_edition)
{
	return _gebr_gui_help_edit(GEBR_GEOXML_OBJECT(document), edited_callback, refresh_callback, menu_edition);
}

GtkWidget* gebr_gui_program_help_edit(GebrGeoXmlProgram * program, GebrGuiHelpEdited edited_callback,
				      GebrGuiHelpRefresh refresh_callback)
{
	return _gebr_gui_help_edit(GEBR_GEOXML_OBJECT(program), edited_callback, refresh_callback, TRUE);
}

/*
 * Private functions
 */

/**
 * \internal
 * Generate Links index.
 */
static void generate_menu_links_index(struct help_data * data)
{
	GString *js = g_string_new(NULL);
	
	if (gebr_geoxml_object_get_type(data->object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) {
		g_string_assign(js, "GenerateLinksIndex([['Menu', 'gebr://menu']]);");
		gebr_js_evaluate(data->context, js->str);
	} else if (gebr_geoxml_object_get_type(data->object) == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
		g_string_assign(js, "GenerateLinksIndex([");
		GebrGeoXmlSequence *program;
		gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(data->object), &program, 0);
		for (gint i = 0; program != NULL; gebr_geoxml_sequence_next(&program), ++i)
			g_string_append_printf(js, "['Program \\'%s\\'', 'gebr://prog%d']",
					       gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)), i);
		g_string_append(js, "]);");
		gebr_js_evaluate(data->context, js->str);
	}

	g_string_free(js, TRUE);
}

/**
 * \internal
 * Save the updated HTML and call edited_callback if set.
 * If the editor is opened remove its HTML code.
 */
static void help_edit_save(struct help_data * data)
{
	GString *help;
	JSValueRef html;

	html = gebr_js_evaluate(data->context, "closeEditor();");
	help = gebr_js_value_get_string(data->context, html);

	/* transform css into a relative path back */
	if (data->menu) {
		regex_t regexp;
		regmatch_t matchptr;
		regcomp(&regexp, "<link[^<]*>", REG_NEWLINE | REG_ICASE);
		if (!regexec(&regexp, help->str, 1, &matchptr, 0)) {
			g_string_erase(help, (gssize) matchptr.rm_so,
				       (gssize) matchptr.rm_eo - matchptr.rm_so);
			g_string_insert(help, (gssize) matchptr.rm_so,
					"<link rel=\"stylesheet\" type=\"text/css\" href=\"gebr.css\" />");
		} else {
			regcomp(&regexp, "<head>", REG_NEWLINE | REG_ICASE);
			if (!regexec(&regexp, help->str, 1, &matchptr, 0))
				g_string_insert(help, (gssize) matchptr.rm_eo,
						"\n  <link rel=\"stylesheet\" type=\"text/css\" href=\"gebr.css\" />");
		}
	}

	if (gebr_geoxml_object_get_type(data->object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		gebr_geoxml_program_set_help(GEBR_GEOXML_PROGRAM(data->object), help->str);
	else
		gebr_geoxml_document_set_help(GEBR_GEOXML_DOCUMENT(data->object), help->str);

	if (data->edit->edited_callback)
		data->edit->edited_callback(data->object, help->str);

	g_string_free(help, TRUE);
}

/**
 * \internal
 * Remove every ocurrence of \p data of #jscontext_to_data_hash
 */
static gboolean hash_foreach_remove(gpointer key, struct help_data * value, struct help_data * data)
{
	return (value == data) ? TRUE : FALSE;
}

/**
 * \internal
 * Update #jscontext_to_data_hash freeing memory alocated related to \p web_view
 */
static void web_view_on_destroy(WebKitWebView * web_view, struct help_data * data)
{
	g_hash_table_foreach_remove(jscontext_to_data_hash, (GHRFunc)hash_foreach_remove, data);
	if (!g_hash_table_size(jscontext_to_data_hash)) {
		g_hash_table_unref(jscontext_to_data_hash);
		jscontext_to_data_hash = NULL;
	}
}

static WebKitNavigationResponse web_view_on_navigation_requested(WebKitWebView * web_view, WebKitWebFrame * frame,
								 WebKitNetworkRequest * request, struct help_data * data)
{
	const gchar * uri = webkit_network_request_get_uri(request);

	if (g_str_has_prefix(uri, "gebr://")) {
		/* only enable in non edition mode and for menus */
		if (!(data->edit == NULL && data->menu))
			return WEBKIT_NAVIGATION_RESPONSE_IGNORE;

		const gchar *help;
		GebrGeoXmlObject *object;
		if (!strcmp(uri, "gebr://menu")) {
			if (gebr_geoxml_object_get_type(data->object) == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						       	_("Invalid link"), _("Sorry, couldn't reach link."));
				return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
			}
			GebrGeoXmlDocument *menu = gebr_geoxml_object_get_owner_document(data->object);
			help = gebr_geoxml_document_get_help(menu);
			object = GEBR_GEOXML_OBJECT(menu);
		} else {
			if (gebr_geoxml_object_get_type(data->object) != GEBR_GEOXML_OBJECT_TYPE_FLOW) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						       	_("Invalid link"), _("Sorry, couldn't reach link."));
				return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
			}
			int program_index = -1;
			sscanf(strstr(uri, "prog"), "prog%d", &program_index);
			GebrGeoXmlSequence *program;
			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(data->object), &program, program_index);
			if (program == NULL) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						       	_("Invalid link"), _("Sorry, couldn't find program."));
				return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
			}

			help = gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(program));
			object = GEBR_GEOXML_OBJECT(program);
		}

		if (!strlen(help)) {
			gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						_("No help available"), _("Sorry, the help is empty."));
			return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
		}
		data->object = object;
		__gebr_gui_help_save_custom_help_to_file(data, help);
		g_signal_connect(data->web_view, "load-finished", G_CALLBACK(web_view_on_load_finished), data);
		webkit_web_view_open(WEBKIT_WEB_VIEW(data->web_view), data->html_path->str);

		return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
	}
	if (g_str_has_prefix(uri, "file://") || g_str_has_prefix(uri, "about:"))
		return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;

	gebr_gui_show_uri(uri);

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
static GtkWidget *web_view_on_create_web_view(GtkDialog ** r_window, struct help_data * data)
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
#if WEBKIT_CHECK_VERSION(1,1,13)
	g_object_set(G_OBJECT(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(web_view))), "enable-universal-access-from-file-uris", TRUE, NULL);
#endif

	/* Place the WebKitWebView in the GtkScrolledWindow */
	gtk_container_add(GTK_CONTAINER(scrolled_window), web_view);
#if GTK_CHECK_VERSION(2,14,0)
	content_area = gtk_dialog_get_content_area(GTK_DIALOG(window));
#else
	content_area = GTK_DIALOG(window)->vbox;
#endif
	if (data) {
		data->edit->actions = actions = gtk_action_group_new("HelpEdit");
		gtk_action_group_add_actions(actions, help_edit_actions, G_N_ELEMENTS(help_edit_actions), data);
		gtk_action_group_add_toggle_actions(actions, help_edit_toggle_actions,
						    G_N_ELEMENTS(help_edit_toggle_actions), data);

		ui = gtk_ui_manager_new();
		gtk_ui_manager_insert_action_group(ui, actions, 0);
		gtk_window_add_accel_group(GTK_WINDOW(window), gtk_ui_manager_get_accel_group(ui));
		if (!gtk_ui_manager_add_ui_from_string(ui, help_edit_ui_manager, -1, &error)) {
			g_message("Failed building gui: %s", error->message);
			g_error_free(error);
		}
		GtkWidget * toolbar = gtk_ui_manager_get_widget(ui, "/HelpToolBar");
		gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
		if (!data->edit->refresh_callback) {
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
static void on_dialog_response(struct help_data * data)
{
	if (data->edit) {
		if (JSValueToBoolean(data->context, gebr_js_evaluate(data->context, "!isContentSaved();"))) {
			gboolean response;
			response = gebr_gui_confirm_action_dialog(_("Save changes in help?"),
								  _("This help has unsaved changes. Do you want to save it?"));
			if (response)
				help_edit_save(data);
		}
	}

	g_unlink(data->html_path->str);
	g_string_free(data->html_path, TRUE);
	if (data->edit) {
		g_object_unref(data->edit->actions);
		g_free(data->edit);
	}
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
 * helpJSFinished, called at the end of help.js.
 */
static JSValueRef helpJSFinished_callback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
					  size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	struct help_data * data = g_hash_table_lookup(jscontext_to_data_hash, (gpointer)function);
	if (data->menu)
		generate_menu_links_index(data);

	return JSValueMakeUndefined(ctx);
}

/**
 * \internal
 * Load all page personalization for editing.
 */
static void web_view_on_load_finished(WebKitWebView * web_view, WebKitWebFrame * frame, struct help_data * data)
{
	data->web_view = web_view;
	data->context = webkit_web_frame_get_global_context(webkit_web_view_get_main_frame(data->web_view));

	/* called by help.js */
	JSObjectRef obj = gebr_js_make_function(data->context, "helpJSFinished", helpJSFinished_callback);
	g_hash_table_insert(jscontext_to_data_hash, (gpointer)obj, data);

	if (data->edit) {
		gchar * script = g_strdup_printf("var menu_refresh = %s;", data->edit->menu_refresh ? "true" : "false");
		gebr_js_evaluate(data->context, script);
		data->edit->menu_refresh = FALSE;
		g_free(script);
		if (data->menu)
			gebr_js_evaluate(data->context, "var menu_edition = true;");
		else
			gebr_js_evaluate(data->context, "var menu_edition = false;");

		gebr_js_evaluate(data->context, start_js);
		gebr_js_evaluate(data->context, "addScript('ckeditor');");
	} else if (data->menu)
		gebr_js_evaluate(data->context, start_js);

	g_signal_handlers_disconnect_matched(G_OBJECT(web_view), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
					     G_CALLBACK(web_view_on_load_finished), data);
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
static gboolean web_view_on_key_press(GtkWidget * widget, GdkEventKey * event, struct help_data * data)
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
 * \internal
 */
static void __gebr_gui_help_save_custom_help_to_file(struct help_data * data, const gchar *_help)
{
	GString *help = g_string_new(_help);

	if (!data->html_path->len) {
		GString *tmp;
		tmp = gebr_make_temp_filename("XXXXXX.html");
		g_string_assign(data->html_path, tmp->str);
		g_string_free(tmp, TRUE);
	}

	/* CSS to absolute path */
	if (help->len) {
		regex_t regexp;
		regmatch_t matchptr;
		regcomp(&regexp, "<link[^<]*gebr.css[^<]*>", REG_NEWLINE | REG_ICASE);
		if (!regexec(&regexp, help->str, 1, &matchptr, 0)) {
			g_string_erase(help, (gssize) matchptr.rm_so,
				       (gssize) matchptr.rm_eo - matchptr.rm_so);
			g_string_insert(help, (gssize) matchptr.rm_so,
					"<link rel=\"stylesheet\" type=\"text/css\" href=\"file://"LIBGEBR_DATA_DIR"/gebr.css\" />");
		} else if (data->menu) {
			regcomp(&regexp, "<head>", REG_NEWLINE | REG_ICASE);
			if (!regexec(&regexp, help->str, 1, &matchptr, 0))
				g_string_insert(help, (gssize) matchptr.rm_eo,
						"\n  <link rel=\"stylesheet\" type=\"text/css\" href=\"file://"LIBGEBR_DATA_DIR"/gebr.css\" />");
		}
	}

	/* some webkit versions crash to open an empty file... */
	if (!help->len)
		g_string_assign(help, " ");

	/* write current help to temporary file */
	FILE *fp;
	fp = fopen(data->html_path->str, "w");
	fputs(help->str, fp);
	fclose(fp);

	g_string_free(help, TRUE);
}

/**
 * \internal
 */
static void __gebr_gui_help_save_help_to_file(struct help_data * data)
{
	const gchar *help;
	if (gebr_geoxml_object_get_type(data->object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		help = gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(data->object));
	else
		help = gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(data->object));

	__gebr_gui_help_save_custom_help_to_file(data, help);
}

/**
 * \internal
 * Load help into a temporary file and load with Webkit (if enabled).
 */
static GtkWidget* _gebr_gui_help_edit(GebrGeoXmlObject * object, GebrGuiHelpEdited edited_callback,
				      GebrGuiHelpRefresh refresh_callback, gboolean menu_edition)
{
	struct help_data *data;
	GtkWidget * web_view;
	GtkDialog * dialog;

	data = g_new(struct help_data, 1);
	data->object = object;
	data->html_path = g_string_new("");
	data->menu = menu_edition;
	data->edit = g_new(struct help_edit_data, 1);
	data->edit->edited_callback = edited_callback;
	data->edit->refresh_callback = refresh_callback;
	data->edit->menu_refresh = FALSE;

	__gebr_gui_help_save_help_to_file(data);

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
	webkit_web_view_open(WEBKIT_WEB_VIEW(web_view), data->html_path->str);

	return GTK_WIDGET(dialog);
}

/**
 * \internal
 */
static void on_help_edit_save_activate(GtkAction * action, struct help_data * data)
{
	help_edit_save(data);
}

/**
 * \internal
 */
static void on_help_edit_edit_toggled(GtkToggleAction * action, struct help_data * data)
{
	gboolean active;

	active = gtk_toggle_action_get_active(action);
	gtk_action_set_sensitive(gtk_action_group_get_action(data->edit->actions, "save"), active);
	gtk_action_set_sensitive(gtk_action_group_get_action(data->edit->actions, "refresh"), active);

	gebr_js_evaluate(data->context, "toggleEditor();");
}

/**
 * \internal
 */
static void on_help_edit_refresh_activate(GtkAction * action, struct help_data * data)
{
	if (data->edit->refresh_callback) {
		data->edit->menu_refresh = TRUE;
		JSValueRef value = gebr_js_evaluate(data->context, "refreshEditor();");
		GString * help;
		help = gebr_js_value_get_string(data->context, value);
		data->edit->refresh_callback(help, data->object);
		__gebr_gui_help_save_custom_help_to_file(data, help->str);
		g_string_free(help, TRUE);

		g_signal_connect(data->web_view, "load-finished", G_CALLBACK(web_view_on_load_finished), data);
		webkit_web_view_open(WEBKIT_WEB_VIEW(data->web_view), data->html_path->str);
	}
}
