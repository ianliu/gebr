/*   libgebr - GêBR Library
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

/**
 * SECTION:gebr-js
 * @short_description: convenience functions for working with JavaScript
 * @title: GêBR JavaScript Module
 * @stability: Stable
 * @see_also: #WebKitWebView, #WebKitWebFrame
 * @include: libgebr/gui/gebr-js.h
 *
 * This module provides convenience functions for evaluating JavaScript scripts and converting JavaScript values into
 * GLib values. The example below assumes you already have a #JSContextRef, which can be retrieved from a #WebKitWebView
 * from the GtkWebKit package.
 *
 * <example>
 * <title></title>
 * <programlisting>
 * {
 *   JSContextRef context;
 *   JSValueRef jsvalue;
 *   gchar * value;
 *
 *   // ...
 *
 *   jsvalue = gebr_js_evaluate(context, "'Hi'");
 *   value = gebr_js_value_get_string(context, jsvalue);
 *   puts(value); // Hi
 *   g_free(value);
 * }
 * </programlisting>
 * </example>
 */

#ifndef __GEBR_GUI_JS_H__
#define __GEBR_GUI_JS_H__

#include <glib.h>
#include <JavaScriptCore/JavaScript.h>

G_BEGIN_DECLS

/**
 * gebr_js_eval_with_url:
 * @ctx: A #JSContextRef, from a #WebKitWebView for example.
 * @script: The JavaScript script to be evaluated.
 * @url: The address associated with this script evaluation.
 *
 * Evaluates @script at the given @url.
 *
 * Returns: a JSValueRef containing the return value of the evaluated script.
 */
JSValueRef gebr_js_eval_with_url(JSContextRef ctx, const gchar * script, const gchar * url);

/**
 * gebr_js_evaluate:
 * @ctx: A #JSContextRef, from a #WebKitWebView for example.
 * @script: The JavaScript script to be evaluated.
 *
 * Evaluates the JavaScript given by @script.
 *
 * Returns: a JSValueRef containing the return value of the evaluated script.
 */
JSValueRef gebr_js_evaluate(JSContextRef ctx, const gchar * script);

/**
 * gebr_js_evaluate_file:
 * @ctx: A #JSContextRef, from a #WebKitWebView for example.
 * @file: A system file path, pointing to the JavaScript file.
 *
 * Evaluates the JavaScript file given by @file.
 *
 * Returns: a JSValueRef containing the return value of the evaluated script.
 */
JSValueRef gebr_js_evaluate_file(JSContextRef ctx, const gchar * file);

/**
 * gebr_js_value_to_string:
 * @ctx: A #JSContextRef, from a #WebKitWebView for example.
 * @value: The value to get the string representation.
 *
 * This function returns the string representation of @value. It is the same as calling the 'toString' method in
 * JavaScript.
 *
 * <note>
 * 	<para>Applying this function into a JSValueRef that is a JavaScript String will return the string itself. But
 * 	you should not trust this behavior. Use gebr_js_value_get_string() for this purpose instead.</para>
 * </note>
 *
 * Returns: A newly allocated string representing @value.
 */
gchar * gebr_js_value_to_string(JSContextRef ctx, JSValueRef value);

/**
 * gebr_js_value_get_string:
 * @ctx: A #JSContextRef, from a #WebKitWebView for example.
 * @val: The value to get the string value.
 *
 * Gets the string value of @val and returns it as a #GString.
 *
 * Returns: A #GString containing the string value of @val.
 */
GString * gebr_js_value_get_string(JSContextRef ctx, JSValueRef val);

/**
 * gebr_js_make_function:
 * @name: The JavaScript function name to be created.
 * @callback: A c-function which will be called upon JavaScript's function call.
 *
 * Creates a JavaScript function in context @ctx which calls @callback.
 *
 * Returns: A JavaScript object representing the function.
 */
JSObjectRef gebr_js_make_function(JSContextRef ctx, const gchar * name, JSObjectCallAsFunctionCallback callback);

/**
 * gebr_js_include:
 * @ctx: A JavaScript context to have @file included.
 * @file: The path of the JavaScript file to be included.
 *
 * Includes a JavaScript file into a context by appending a <emphasis>%lt;script%gt;</emphasis> tag into
 * <emphasis>document.body</emphasis>. You must guarantee body tag exists, otherwise this function may fail silently.
 * This might be done by connecting to "load-finished" signal of #WebKitWebView.
 *
 * Returns: %TRUE if @file exists, %FALSE otherwise.
 */
gboolean gebr_js_include(JSContextRef ctx, const gchar * file);

G_END_DECLS

#endif /* __GEBR_GUI_JS_H__ */
