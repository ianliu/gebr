/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <regex.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <webkit/webkit.h>
#include <libgebr/utils.h>
#include <locale.h>
#include <libgebr/geoxml/geoxml.h>

#include "debr-help-edit-widget.h"

/*
 * HTML_HOLDER:
 * Defines the HTML containing the textarea that will load the CKEditor.
 */
#define HTML_HOLDER									\
	"<html>"									\
	"<head>"									\
	"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"	\
	"<script>"									\
	"var menu_edition = false;"							\
	"function onCkEditorLoadFinished() {}"						\
	"</script>"									\
	"<script src='" DEBR_HELP_EDIT_CKEDITOR "'>"					\
	"</script>"									\
	"</head>"                                                                       \
	"<body>"									\
	"<textarea id=\"editor\" name=\"editor\"></textarea>"				\
	"<script>"									\
	"editor = CKEDITOR.replace('editor', {"						\
	"	height:300,"								\
	"	width:'99%%',"								\
	"	resize_enabled:false,"							\
	"	toolbarCanCollapse:false,"						\
	"	toolbar:["								\
	"		['Source'],"							\
	"		['Bold','Italic','Underline'],"					\
	"		['Subscript','Superscript'],"					\
	"		['Undo','Redo'],"						\
	"		['JustifyLeft','JustifyCenter','JustifyRight','JustifyBlock'],"	\
	"		['NumberedList','BulletedList'],"				\
	"		['Outdent','Indent','Blockquote','Styles'],"			\
	"		['Link','Unlink'],"						\
	"		['RemoveFormat'],"						\
	"		['Find','Replace','Table']"					\
	"	]"									\
	"});"										\
	"</script>"									\
	"</body>"									\
	"</html>"									\
	""

enum {
	PROP_0,
	PROP_OBJECT
};

typedef struct _DebrHelpEditWidgetPrivate DebrHelpEditWidgetPrivate;

struct _DebrHelpEditWidgetPrivate {
	GebrGeoXmlObject * object;
	gchar * temp_file;

	// This variable is set to TRUE when the content is commited, and to FALSE when the content is set.
	gboolean is_commited;
};

#define DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), DEBR_TYPE_HELP_EDIT_WIDGET, DebrHelpEditWidgetPrivate))

#define CSS_LINK "<link rel=\"stylesheet\" type=\"text/css\" href=\"file://"LIBGEBR_DATA_DIR"/gebr.css\" />"

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void debr_help_edit_widget_set_property	(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);

static void debr_help_edit_widget_get_property	(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);

static void debr_help_edit_widget_finalize(GObject *object);

static void debr_help_edit_widget_commit_changes(GebrGuiHelpEditWidget * self);

static gchar * debr_help_edit_widget_get_content(GebrGuiHelpEditWidget * self);

static void debr_help_edit_widget_set_content(GebrGuiHelpEditWidget * self, const gchar * content);

static gboolean debr_help_edit_widget_is_content_saved(GebrGuiHelpEditWidget * self);

static const gchar *debr_help_edit_widget_get_uri (GebrGuiHelpEditWidget *self);

static gboolean check_editor_dirty(GebrGuiHelpEditWidget * self);

static void reset_editor_dirty(GebrGuiHelpEditWidget * self);

static gchar *get_raw_content (GebrGuiHelpEditWidget *self);

G_DEFINE_TYPE(DebrHelpEditWidget, debr_help_edit_widget, GEBR_GUI_TYPE_HELP_EDIT_WIDGET);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void debr_help_edit_widget_class_init (DebrHelpEditWidgetClass *klass)
{
	GObjectClass *gobject_class;
	GebrGuiHelpEditWidgetClass *super_class;

	gobject_class = G_OBJECT_CLASS(klass);
	super_class = GEBR_GUI_HELP_EDIT_WIDGET_CLASS(klass);

	gobject_class->set_property = debr_help_edit_widget_set_property;
	gobject_class->get_property = debr_help_edit_widget_get_property;
	gobject_class->finalize = debr_help_edit_widget_finalize;
	super_class->commit_changes = debr_help_edit_widget_commit_changes;
	super_class->get_content = debr_help_edit_widget_get_content;
	super_class->set_content = debr_help_edit_widget_set_content;
	super_class->is_content_saved = debr_help_edit_widget_is_content_saved;
	super_class->get_uri = debr_help_edit_widget_get_uri;

	/**
	 * DebrHelpEditWidget:geoxml-object:
	 * The #GebrGeoXmlObject associated with this help edition.
	 */
	g_object_class_install_property(gobject_class,
					PROP_OBJECT,
					g_param_spec_pointer("geoxml-object",
							     "GeoXml Object",
							     "The GebrGeoXmlObject associated with this help edition.",
							     G_PARAM_READWRITE));

	g_type_class_add_private(klass, sizeof(DebrHelpEditWidgetPrivate));
}

static void debr_help_edit_widget_init(DebrHelpEditWidget * self)
{
	DebrHelpEditWidgetPrivate * priv;

	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	priv->is_commited = FALSE;
	priv->object = NULL;
	priv->temp_file = NULL;
}

static void debr_help_edit_widget_set_property(GObject		*object,
					       guint		 prop_id,
					       const GValue	*value,
					       GParamSpec	*pspec)
{
	GebrGuiHelpEditWidget *self;
	DebrHelpEditWidgetPrivate *priv;
	GebrGuiHtmlViewerWidget *html_viewer;

	self = GEBR_GUI_HELP_EDIT_WIDGET (object);
	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE (object);
	html_viewer = gebr_gui_help_edit_widget_get_html_viewer (self);

	switch (prop_id) {
	case PROP_OBJECT:
		priv->object = GEBR_GEOXML_OBJECT (g_value_get_pointer (value));
		gebr_gui_html_viewer_widget_generate_links (html_viewer, priv->object);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void debr_help_edit_widget_get_property(GObject		*object,
					       guint		 prop_id,
					       GValue		*value,
					       GParamSpec	*pspec)
{
	DebrHelpEditWidgetPrivate * priv;
	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_OBJECT:
		g_value_set_pointer(value, priv->object);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void debr_help_edit_widget_finalize(GObject *object)
{
	DebrHelpEditWidgetPrivate * priv;

	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(object);
	if (priv->temp_file) {
		g_unlink(priv->temp_file);
		g_free(priv->temp_file);
	}

	G_OBJECT_CLASS(debr_help_edit_widget_parent_class)->finalize(object);
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================
static void pre_process_html (GString *html)
{
	gchar *content;
	gchar *escaped;
	content = gebr_geoxml_object_get_help_content_from_str (html->str);
	escaped = gebr_str_escape (content);
	g_string_assign (html, escaped);
	g_free (content);
	g_free (escaped);
}

static void on_load_finished(WebKitWebView * view, WebKitWebFrame * frame, GebrGuiHelpEditWidget *self)
{
	gchar *content;

	// Disconnects ourselves to prevent subsequent load-finished calls.
	g_signal_handlers_disconnect_by_func(view,
					     on_load_finished,
					     self);

	content = g_object_get_data (G_OBJECT (self), "content");
	gebr_gui_help_edit_widget_set_content (self, content);
	g_free (content);
}

static gboolean check_editor_dirty(GebrGuiHelpEditWidget * self)
{
	JSContextRef context;
	JSValueRef value;
	context = gebr_gui_help_edit_widget_get_js_context(self);
	value = gebr_js_evaluate(context, "editor.checkDirty();");
	return gebr_js_value_get_boolean(context, value);
}

static void reset_editor_dirty(GebrGuiHelpEditWidget * self)
{
	JSContextRef context;
	context = gebr_gui_help_edit_widget_get_js_context(self);
	gebr_js_evaluate(context, "editor.resetDirty();");
}

/*
 * Fetches the content from within the editor, not the generated help.
 * See debr_help_edit_widget_get_content() for fetching the hole help string.
 */
static gchar *get_raw_content (GebrGuiHelpEditWidget *self)
{
	JSContextRef context;
	JSValueRef value;
	GString *str;

	context = gebr_gui_help_edit_widget_get_js_context (self);
	value = gebr_js_evaluate (context, "editor.getData();");
	str = gebr_js_value_get_string (context, value);
	return g_string_free (str, FALSE);
}

//==============================================================================
// IMPLEMENTATION OF ABSTRACT FUNCTIONS					       =
//==============================================================================
static void debr_help_edit_widget_commit_changes(GebrGuiHelpEditWidget * self)
{
	gchar * content;
	DebrHelpEditWidgetPrivate * priv;

	content = gebr_gui_help_edit_widget_get_content (self);
	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE (self);

	switch (gebr_geoxml_object_get_type (priv->object)) {
	case GEBR_GEOXML_OBJECT_TYPE_PROGRAM:
		gebr_geoxml_program_set_help(GEBR_GEOXML_PROGRAM(priv->object), content);
		break;
	case GEBR_GEOXML_OBJECT_TYPE_FLOW:
		gebr_geoxml_document_set_help(GEBR_GEOXML_DOCUMENT(priv->object), content);
		break;
	default:
		g_free(content);
		g_return_if_reached();
	}

	reset_editor_dirty (self);
	priv->is_commited = TRUE;
	g_free(content);
}

static gchar *debr_help_edit_widget_get_content (GebrGuiHelpEditWidget *self)
{
	gchar *help;
	gchar *raw;
	DebrHelpEditWidgetPrivate *priv;

	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE (self);
	raw = get_raw_content (self);
	help = gebr_geoxml_object_generate_help (priv->object, raw);
	g_free (raw);

	return help;
}

static void debr_help_edit_widget_set_content(GebrGuiHelpEditWidget * self, const gchar *content)
{
	gchar *script;
	GString *cont;
	JSContextRef context;

	cont = g_string_new (content);
	pre_process_html (cont);
	script = g_strdup_printf ("editor.setData(\"%s\");", cont->str);
	context = gebr_gui_help_edit_widget_get_js_context(self);
	gebr_js_evaluate(context, script);

	g_free (script);
	g_string_free (cont, TRUE);
}

static gboolean debr_help_edit_widget_is_content_saved(GebrGuiHelpEditWidget * self)
{
	DebrHelpEditWidgetPrivate * priv;
	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	return priv->is_commited && !check_editor_dirty(self);
}

static const gchar *debr_help_edit_widget_get_uri (GebrGuiHelpEditWidget *self)
{
	DebrHelpEditWidgetPrivate *priv;

	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE (self);
	return priv->temp_file;
}

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================
GebrGuiHelpEditWidget * debr_help_edit_widget_new(GebrGeoXmlObject * object,
						  const gchar * content,
						  gboolean committed)
{
	GError *error = NULL;
	GString * temp_file;
	GtkWidget * web_view;
	GebrGuiHelpEditWidget * self;
	DebrHelpEditWidgetPrivate * priv;

	self = g_object_new(DEBR_TYPE_HELP_EDIT_WIDGET,
			    "geoxml-object", object,
			    NULL);

	// Creates a temporary file which will live until this instance lives.
	// This temporary file is used to write the Help string into it so webkit can display it.
	priv = DEBR_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	temp_file = gebr_make_temp_filename("XXXXXX.html");
	priv->temp_file = g_string_free(temp_file, FALSE);
	web_view = gebr_gui_help_edit_widget_get_web_view (self);

	g_file_set_contents (priv->temp_file, HTML_HOLDER, -1, &error);
	if (error) {
		g_warning ("DebrHelpEditWidget: %s", error->message);
		g_clear_error (&error);
		return self;
	}

	g_object_set_data (G_OBJECT (self), "content", g_strdup (content));

	g_signal_connect (web_view, "load-finished",
			  G_CALLBACK (on_load_finished), self);

	// On newer version of WebKit there is a security option we must disable
	// so WebKit can load the external JavaScript file properly.
#if WEBKIT_CHECK_VERSION(1,1,13)
	g_object_set(G_OBJECT(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(web_view))),
		     "enable-universal-access-from-file-uris", TRUE, NULL);
#endif

	webkit_web_view_open (WEBKIT_WEB_VIEW (web_view), priv->temp_file);
	priv->is_commited = committed;

	return self;
}

gboolean debr_help_edit_widget_is_content_empty(DebrHelpEditWidget * self)
{
	gboolean retval;
	GString * string;
	JSValueRef value;
	JSContextRef context;
	GebrGuiHelpEditWidget * super;

	g_return_val_if_fail (DEBR_IS_HELP_EDIT_WIDGET (self), TRUE);

	super = GEBR_GUI_HELP_EDIT_WIDGET (self);
	context = gebr_gui_help_edit_widget_get_js_context (super);
	value = gebr_js_evaluate (context, "editor.getData();");
	string = gebr_js_value_get_string (context, value);

	if (string->len > 0)
		retval = FALSE;
	else
		retval = TRUE;

	g_string_free (string, TRUE);

	return retval;
}
