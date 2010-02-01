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

#include <JavaScriptCore/JavaScript.h>
#include <glib.h>

G_BEGIN_DECLS

JSValueRef gebr_js_evaluate(JSContextRef ctx, const gchar * script);

gchar * gebr_js_value_to_string(JSContextRef ctx, JSValueRef value);

GString * gebr_js_value_get_string(JSContextRef ctx, JSValueRef val);

JSValueRef gebr_js_make_function(JSContextRef ctx, const gchar * name, JSObjectCallAsFunctionCallback callback);

G_END_DECLS

#endif /* __GEBR_GUI_JS_H__ */
