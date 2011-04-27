#include "gebr-string-expr.h"
#include "gebr-iexpr.h"

static void gebr_string_expr_interface_init(GebrIExprInterface *iface);

G_DEFINE_TYPE_WITH_CODE(GebrStringExpr, gebr_string_expr, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE(GEBR_TYPE_IEXPR,
					      gebr_string_expr_interface_init));

static void gebr_string_expr_class_init(GebrStringExprClass *klass)
{
}

static void gebr_string_expr_init(GebrStringExpr *self)
{
}

static gboolean gebr_string_expr_set_var(GebrIExpr              *self,
					 const gchar            *expr,
					 GebrGeoXmlParameterType type,
					 const gchar            *value,
					 GError                **err)
{
	return TRUE;
}

static gboolean gebr_string_expr_is_valid(GebrIExpr   *self,
					  const gchar *expr,
					  GError     **err)
{
	return TRUE;
}

static void gebr_string_expr_interface_init(GebrIExprInterface *iface)
{
	iface->set_var = gebr_string_expr_set_var;
	iface->is_valid = gebr_string_expr_is_valid;
}

GebrStringExpr *gebr_string_expr_new(void)
{
	return g_object_new(GEBR_TYPE_STRING_EXPR, NULL);
}

gboolean gebr_string_expr_eval(GebrStringExpr *self,
			       const gchar *expr,
			       GError **error)
{
	return TRUE;
}
