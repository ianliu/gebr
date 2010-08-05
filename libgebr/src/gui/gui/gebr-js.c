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

#include <glib/gprintf.h>
#include <JavaScriptCore/JSValueRef.h>

#include "gebr-js.h"

/*
 * Private functions
 */
static gchar * gebr_js_string_to_cstring(JSStringRef string);

/*
 * Public functions
 */

JSValueRef gebr_js_eval_with_url(JSContextRef ctx, const gchar * script, const gchar * url)
{
	JSObjectRef obj;
	JSStringRef script_str;
	JSStringRef source;
	JSValueRef val;
	JSValueRef except;

	except = NULL;
	script_str = JSStringCreateWithUTF8CString(script);
	source = url? JSStringCreateWithUTF8CString(url) : NULL;
	obj = JSContextGetGlobalObject(ctx);
	val = JSEvaluateScript(ctx, script_str, obj, source, 0, &except);
	if (except) {
		JSPropertyNameArrayRef array;
		g_warning("[JavaScript exception]");
		array = JSObjectCopyPropertyNames(ctx, (JSObjectRef)except);
		for (size_t i = 0; i < JSPropertyNameArrayGetCount(array); i++) {
			gchar * prop_name;
			gchar * prop_value;
			JSValueRef value;
			JSStringRef name_str;

			name_str = JSPropertyNameArrayGetNameAtIndex(array, i);
			value = JSObjectGetProperty(ctx, (JSObjectRef)except, name_str, NULL);

			prop_name = gebr_js_string_to_cstring(name_str);
			prop_value = gebr_js_value_to_string(ctx, value);

			g_fprintf(stderr, "  %s => %s\n", prop_name, prop_value);

			g_free(prop_name);
			g_free(prop_value);
			JSStringRelease(name_str);
		}
		JSPropertyNameArrayRelease(array);
	}
	JSStringRelease(script_str);
	if (url)
		JSStringRelease(source);

	return val;
}

JSValueRef gebr_js_evaluate(JSContextRef ctx, const gchar * script)
{
	return gebr_js_eval_with_url(ctx, script, NULL);
}

JSValueRef gebr_js_evaluate_file(JSContextRef ctx, const gchar * file)
{
	gsize size;
	gchar * str;
	GError * error;
	GIOChannel * channel;
	GString * string;
	JSValueRef value;

	error = NULL;
	channel = g_io_channel_new_file(file, "r", &error);
	g_io_channel_read_to_end(channel, &str, &size, &error);
	g_io_channel_shutdown(channel, FALSE, &error);
	string = g_string_new("file://");
	g_string_append(string, file);
	value = gebr_js_eval_with_url(ctx, str, string->str);
	g_free(str);
	g_string_free(string, TRUE);

	return value;
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
	len = JSStringGetMaximumUTF8CStringSize(str);
	buf = g_new(gchar, len);
	JSStringGetUTF8CString(str, buf, len);
	JSStringRelease(str);
	ret = g_string_new(buf);
	g_free(buf);

	return ret;
}

JSObjectRef gebr_js_make_function(JSContextRef ctx, const gchar * name, JSObjectCallAsFunctionCallback callback)
{
	JSObjectRef ret;
	JSStringRef name_str;
	
	name_str = JSStringCreateWithUTF8CString(name);
	ret = JSObjectMakeFunctionWithCallback(ctx, name_str, callback);
	JSObjectSetProperty(ctx, JSContextGetGlobalObject(ctx), name_str, ret, kJSPropertyAttributeNone, NULL);

	JSStringRelease(name_str);
	return ret;
}

gboolean gebr_js_include(JSContextRef ctx, const gchar * file)
{
	if (!g_file_test(file, G_FILE_TEST_EXISTS))
		return FALSE;
	GString * script;
	const gchar * include_script =
		"(function() {"
			"var script = document.createElement('script');"
			"script.setAttribute('type', 'text/javascript');"
			"script.setAttribute('src', 'file://%s');"
			"document.body.appendChild(script);"
		"})();";
	script = g_string_new(NULL);
	g_string_printf(script, include_script, file);
	gebr_js_evaluate(ctx, script->str);
	g_string_free(script, TRUE);

	return TRUE;
}

static gchar * gebr_js_string_to_cstring(JSStringRef string)
{
	size_t len;
	gchar * buffer;

	len = JSStringGetMaximumUTF8CStringSize(string);
	buffer = g_new(gchar, len);
	JSStringGetUTF8CString(string, buffer, len);

	return buffer;
}
