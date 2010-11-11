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

#include <JavaScriptCore/JavaScript.h>
#include <glib.h>

#include "../gui/gebr-gui-js.h"

static void
test_gebr_js_value_get_string(void)
{
	GString * string;
	JSValueRef value;
	JSContextRef ctx;
	JSStringRef str;

	ctx = (JSContextRef)JSGlobalContextCreate(NULL);
	str = JSStringCreateWithUTF8CString("test");
	value = JSValueMakeString(ctx, str);
	JSStringRelease(str);

	string = gebr_js_value_get_string(ctx, value);
	g_assert_cmpstr(string->str, ==, "test");

	str = JSStringCreateWithUTF8CString("tést");
	value = JSValueMakeString(ctx, str);
	JSStringRelease(str);

	string = gebr_js_value_get_string(ctx, value);
	g_assert_cmpstr(string->str, ==, "tést");
}

static void
test_gebr_js_evaluate(void)
{
	GString * string;
	JSValueRef value;
	JSContextRef ctx;

	ctx = (JSContextRef)JSGlobalContextCreate(NULL);
	value = gebr_js_evaluate(ctx, "'foo';");
	g_assert(JSValueGetType(ctx, value) == kJSTypeString);

	string = gebr_js_value_get_string(ctx, value);
	g_assert_cmpstr(string->str, ==, "foo");
}

static JSValueRef
callback(JSContextRef context, JSObjectRef function, JSObjectRef thiObject, size_t argc, const JSValueRef args[], JSValueRef * exception)
{
	JSStringRef string;
	JSValueRef value;
	string = JSStringCreateWithUTF8CString("test");
	value = JSValueMakeString(context, string);
	JSStringRelease(string);
	return value;
}

static void
test_gebr_js_make_function(void)
{
	JSValueRef value;
	JSContextRef ctx;

	ctx = (JSContextRef)JSGlobalContextCreate(NULL);
	gebr_js_make_function(ctx, "testFunc", callback);
	value = gebr_js_evaluate(ctx, "testFunc();");
	g_assert(value != NULL);
	g_assert_cmpstr(gebr_js_value_get_string(ctx, value)->str, ==, "test");
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/gui/js/value-get-string", test_gebr_js_value_get_string);
	g_test_add_func("/gui/js/evaluate", test_gebr_js_evaluate);
	g_test_add_func("/gui/js/make-function", test_gebr_js_make_function);

	return g_test_run();
}
