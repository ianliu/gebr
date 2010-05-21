/*   libgebr - G�BR Library
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

#ifndef __GEBR_GUI_JS_H__
#define __GEBR_GUI_JS_H__

#include <glib.h>
#include <JavaScriptCore/JavaScript.h>

G_BEGIN_DECLS

JSValueRef gebr_js_eval_with_url(JSContextRef ctx, const gchar * script, const gchar * url);

JSValueRef gebr_js_evaluate(JSContextRef ctx, const gchar * script);

JSValueRef gebr_js_evaluate_file(JSContextRef ctx, const gchar * file);

gchar * gebr_js_value_to_string(JSContextRef ctx, JSValueRef value);

/**
 *
 */
GString * gebr_js_value_get_string(JSContextRef ctx, JSValueRef val);

/**
 * Creates a JavaScript function in contect \p ctx which calls \p callback.
 * @param name Functions name in JavaScript.
 * @param callback A c-function which will be called upon JavaScript's function call.
 * @return A JavaScript value representing the function.
 */
JSObjectRef gebr_js_make_function(JSContextRef ctx, const gchar * name, JSObjectCallAsFunctionCallback callback);

/**
 * Includes a JavaScript file into a context by appending a <script> tag into document.body.
 * You must guarantee body tag exists, otherwise this function may fail silently. This might be done by connecting to
 * "load-finished" signal of #WebKitWebView.
 *
 * @param ctx A JavaScript context to have \p file included.
 * @param file The path of the JavaScript file to be included; 'file://' is automatically inserted in front of it.
 * @return TRUE if \p file exists, FALSE otherwise.
 */
gboolean gebr_js_include(JSContextRef ctx, const gchar * file);

G_END_DECLS

#endif /* __GEBR_GUI_JS_H__ */
