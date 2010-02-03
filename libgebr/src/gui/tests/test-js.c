#include <glib.h>
#include <webkit/webkit.h>
#include "js.h"

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
	gtk_init(&argc, &argv);

	g_test_add_func("/gui/js/value-get-string", test_gebr_js_value_get_string);
	g_test_add_func("/gui/js/evaluate", test_gebr_js_evaluate);
	g_test_add_func("/gui/js/make-function", test_gebr_js_make_function);

	return g_test_run();
}
