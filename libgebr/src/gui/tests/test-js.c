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

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);
	gtk_init(&argc, &argv);

	g_test_add_func("/gui/js/value-get-string", test_gebr_js_value_get_string);
	g_test_add_func("/gui/js/evaluate", test_gebr_js_evaluate);

	return g_test_run();
}
