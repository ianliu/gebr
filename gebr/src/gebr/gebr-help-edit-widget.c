/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

#include <webkit/webkit.h>
#include <libgebr/utils.h>

#include "gebr-help-edit-widget.h"
#include "document.h"

/*
 * HTML_HOLDER:
 * Defines the HTML containing the textarea that will load the CKEditor.
 */
#define HTML_HOLDER									\
	"<html>"									\
	"<head>"									\
	"<script>"									\
	"var menu_edition = false;"							\
	"function onCkEditorLoadFinished() {}"						\
	"</script>"									\
	"<script src='" HELP_EDIT_SCRIPT_PATH "ckeditor.js'>"				\
	"</script></head>"								\
	"<body>"									\
	"<textarea id=\"editor\" name=\"editor\">%s</textarea>"				\
	"<script>"									\
	"ed = CKEDITOR.replace('editor', {"						\
	"	fullPage:true,"								\
	"	height:300,"								\
	"	width:'99%%',"								\
	"	resize_enabled:false,"							\
	"	toolbarCanCollapse:false,"						\
	"	toolbar:[['Source'],['Bold','Italic','Underline'],"			\
	"		['Subscript','Superscript'],['Undo','Redo'],"			\
	"		['JustifyLeft','JustifyCenter','JustifyRight','JustifyBlock'],"	\
	"		['NumberedList','BulletedList'],['Outdent','Indent','Blockquote','Styles'],"	\
	"		['Link','Unlink'],['RemoveFormat'],['Find','Replace','Table']]"	\
	"});"										\
	"</script>"									\
	"</body>"									\
	"</html>"									\
	""

enum {
	PROP_0,
	PROP_DOCUMENT
};

typedef struct _GebrHelpEditWidgetPrivate GebrHelpEditWidgetPrivate;

struct _GebrHelpEditWidgetPrivate {
	GebrGeoXmlDocument * document;
	gchar * temp_file;
};

#define GEBR_HELP_EDIT_WIDGET_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_TYPE_HELP_EDIT_WIDGET, GebrHelpEditWidgetPrivate))

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void gebr_help_edit_widget_set_property	(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);
static void gebr_help_edit_widget_get_property	(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);
static void gebr_help_edit_widget_destroy(GtkObject *object);

static void gebr_help_edit_widget_commit_changes(GebrGuiHelpEditWidget * self);

static gchar * gebr_help_edit_widget_get_content(GebrGuiHelpEditWidget * self);

static void gebr_help_edit_widget_set_content(GebrGuiHelpEditWidget * self, const gchar * content);

static gboolean gebr_help_edit_widget_is_content_saved(GebrGuiHelpEditWidget * self);

G_DEFINE_TYPE(GebrHelpEditWidget, gebr_help_edit_widget, GEBR_GUI_TYPE_HELP_EDIT_WIDGET);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void gebr_help_edit_widget_class_init(GebrHelpEditWidgetClass * klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GebrGuiHelpEditWidgetClass *super_class;

	gobject_class = G_OBJECT_CLASS(klass);
	object_class = GTK_OBJECT_CLASS(klass);
	super_class = GEBR_GUI_HELP_EDIT_WIDGET_CLASS(klass);

	gobject_class->set_property = gebr_help_edit_widget_set_property;
	gobject_class->get_property = gebr_help_edit_widget_get_property;
	object_class->destroy = gebr_help_edit_widget_destroy;
	super_class->commit_changes = gebr_help_edit_widget_commit_changes;
	super_class->get_content = gebr_help_edit_widget_get_content;
	super_class->set_content = gebr_help_edit_widget_set_content;
	super_class->is_content_saved = gebr_help_edit_widget_is_content_saved;

	/**
	 * GebrHelpEditWidget:geoxml-document:
	 * The #GebrGeoXmlDocument associated with this help edition.
	 */
	g_object_class_install_property(gobject_class,
					PROP_DOCUMENT,
					g_param_spec_pointer("geoxml-document",
							     "GeoXml Document",
							     "The GebrGeoXmlDocument associated with this help edition.",
							     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private(klass, sizeof(GebrHelpEditWidgetPrivate));
}

static void gebr_help_edit_widget_init(GebrHelpEditWidget * self)
{
}

static void gebr_help_edit_widget_set_property(GObject		*object,
					       guint		 prop_id,
					       const GValue	*value,
					       GParamSpec	*pspec)
{
	GebrHelpEditWidgetPrivate * priv;
	priv = GEBR_HELP_EDIT_WIDGET_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_DOCUMENT:
		priv->document = GEBR_GEOXML_DOCUMENT(g_value_get_pointer(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_help_edit_widget_get_property(GObject		*object,
					       guint		 prop_id,
					       GValue		*value,
					       GParamSpec	*pspec)
{
	GebrHelpEditWidgetPrivate * priv;
	priv = GEBR_HELP_EDIT_WIDGET_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_DOCUMENT:
		g_value_set_pointer(value, priv->document);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_help_edit_widget_destroy(GtkObject *object)
{
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================

/*
 * pre_process_html:
 * Pre process the @html string, inserting the CSS header, javascript includes,
 * etc.
 *
 * @html: The #GString to be modified.
 */
static void pre_process_html(GString * html)
{
	// Do nothing for now ;)
}

static void on_load_finished(WebKitWebView * view, WebKitWebFrame * frame)
{
	JSContextRef context;

	context = webkit_web_frame_get_global_context(frame);

	g_signal_handlers_disconnect_by_func(view,
					     on_load_finished,
					     NULL);
}

//==============================================================================
// IMPLEMENTATION OF ABSTRACT FUNCTIONS					       =
//==============================================================================
static void gebr_help_edit_widget_commit_changes(GebrGuiHelpEditWidget * self)
{
	gchar * content;
	JSContextRef context;
	GebrHelpEditWidgetPrivate * priv;

	context = gebr_gui_help_edit_widget_get_js_context(self);
	gebr_js_evaluate(context, "ed.resetDirty();");

	priv = GEBR_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	content = gebr_help_edit_widget_get_content(self);
	gebr_geoxml_document_set_help(priv->document, content);
	document_save(priv->document, TRUE);
	g_free(content);
}

static gchar * gebr_help_edit_widget_get_content(GebrGuiHelpEditWidget * self)
{
	// Executes the JavaScript code:
	//   return ed.getData();

	JSContextRef context;
	JSValueRef value;
	GString * str;

	context = gebr_gui_help_edit_widget_get_js_context(self);
	value = gebr_js_evaluate(context, "ed.getData();");
	str = gebr_js_value_get_string(context, value);
	return g_string_free(str, FALSE);
}

static void gebr_help_edit_widget_set_content(GebrGuiHelpEditWidget * self, const gchar * content)
{
	// Executes the JavaScript code:
	//   ed.setData(content);

	gchar * script;
	JSContextRef context;

	script = g_strdup_printf("ed.setData(%s);", content);
	context = gebr_gui_help_edit_widget_get_js_context(self);
	gebr_js_evaluate(context, script);

	g_free(script);
}

static gboolean gebr_help_edit_widget_is_content_saved(GebrGuiHelpEditWidget * self)
{
	JSContextRef context;
	JSValueRef value;
	context = gebr_gui_help_edit_widget_get_js_context(self);
	value = gebr_js_evaluate(context, "ed.checkDirty();");
	return !gebr_js_value_get_boolean(context, value);
}

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================
GtkWidget * gebr_help_edit_widget_new(GebrGeoXmlDocument * document, const gchar * content)
{
	FILE * fp;
	gchar * escaped;
	GString * help;
	GString * temp_file;
	GtkWidget * web_view;
	GebrGuiHelpEditWidget * self;
	GebrHelpEditWidgetPrivate * priv;

	self = g_object_new(GEBR_TYPE_HELP_EDIT_WIDGET,
			    "geoxml-document", document,
			    NULL);

	escaped = g_markup_escape_text(content, -1);
	help = g_string_new(escaped);
	web_view = gebr_gui_help_edit_widget_get_web_view(self);
	temp_file = gebr_make_temp_filename("XXXXXX.html");
	priv = GEBR_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	priv->temp_file = g_string_free(temp_file, FALSE);

	g_free(escaped);

	if (!help->len)
		g_string_assign(help, " ");

	pre_process_html(help);

	fp = fopen(priv->temp_file, "w");

	/* Return with a warning, if we could not create the temporary file */
	g_return_val_if_fail(fp != NULL, GTK_WIDGET(self));

	/* Return with a warning, if we could not insert content into the
	 * temporary file */
	g_return_val_if_fail(fprintf(fp, HTML_HOLDER, help->str) >= 0, GTK_WIDGET(self));

	g_signal_connect(web_view, "load-finished", G_CALLBACK(on_load_finished), NULL);

	g_object_set(G_OBJECT(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(web_view))),
		     "enable-universal-access-from-file-uris", TRUE, NULL);

	webkit_web_view_open(WEBKIT_WEB_VIEW(web_view), priv->temp_file);

	fclose(fp);

	return GTK_WIDGET(self);
}
