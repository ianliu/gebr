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

#include "js.h"

JSValueRef gebr_js_evaluate(JSContextRef ctx, const gchar * script)
{
	JSObjectRef obj;
	JSStringRef scr;
	JSValueRef val;

	scr = JSStringCreateWithUTF8CString(script);
	obj = JSContextGetGlobalObject(ctx);
	val = JSEvaluateScript(ctx, scr, obj, NULL, 0, NULL);
	JSStringRelease(scr);

	return val;
}

gchar * gebr_js_value_to_string(JSContextRef ctx, JSValueRef value)
{
	gsize len;
	gchar * buf;
	JSStringRef str;

	str = JSValueToStringCopy(ctx, value, NULL);
	len = JSStringGetLength(str);
	buf = g_new(gchar, len + 1);
	JSStringGetUTF8CString(str, buf, len + 1);
	JSStringRelease(str);

	return buf;
}

GString * gebr_js_value_get_string(JSContextRef ctx, JSValueRef val)
{
	gsize len;
	gchar * buf;
	GString * ret;
	JSStringRef str;

	if (!JSValueIsString(ctx, val))
		return NULL;

	str = JSValueToStringCopy(ctx, val, NULL);
	len = JSStringGetLength(str);
	JSStringGetUTF8CString(str, buf, len + 1);
	JSStringRelease(str);
	ret = g_string_new(buf);
	g_free(buf);

	return ret;
}

JSValueRef gebr_js_make_function(JSContextRef ctx, const gchar * name, JSObjectCallAsFunctionCallback callback)
{
	JSValueRef ret;
	JSStringRef name_str;
	name_str = JSStringCreateWithUTF8CString(name);
	ret = JSObjectMakeFunctionWithCallback(ctx, name_str, callback);
	JSStringRelease(name_str);
	return ret;
}

