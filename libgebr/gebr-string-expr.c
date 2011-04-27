#include <glib/gi18n.h>

#include "gebr-string-expr.h"
#include "gebr-iexpr.h"

/* Prototypes & Private {{{ */

struct _GebrStringExprPriv {
	GHashTable *vars;
};

typedef struct {
	gchar *name;
	GebrGeoXmlParameterType type;
} TypedVar;

static void gebr_string_expr_interface_init(GebrIExprInterface *iface);

static void gebr_string_expr_finalize(GObject *object);

static guint hash_func(gconstpointer a);

static gboolean equal_func(gconstpointer a, gconstpointer b);

static void typed_var_free(gpointer data);

TypedVar *typed_var_new(const gchar *name, GebrGeoXmlParameterType type);

G_DEFINE_TYPE_WITH_CODE(GebrStringExpr, gebr_string_expr, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE(GEBR_TYPE_IEXPR,
					      gebr_string_expr_interface_init));

/* }}} */

/* Class & Instance initialize/finalize {{{ */
static void gebr_string_expr_class_init(GebrStringExprClass *klass)
{
	GObjectClass *g_class;

	g_class = G_OBJECT_CLASS(klass);
	g_class->finalize = gebr_string_expr_finalize;

	g_type_class_add_private(klass, sizeof(GebrStringExprPriv));
}

static void gebr_string_expr_init(GebrStringExpr *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
						 GEBR_TYPE_STRING_EXPR,
						 GebrStringExprPriv);

	self->priv->vars = g_hash_table_new_full(hash_func, equal_func,
						 typed_var_free, g_free);
}

static void gebr_string_expr_finalize(GObject *object)
{
	GebrStringExpr *self = GEBR_STRING_EXPR(object);

	g_hash_table_unref(self->priv->vars);
}
/* }}} */

/* Variables hash table functions {{{ */
static guint hash_func(gconstpointer a)
{
	const TypedVar *v = a;
	return g_str_hash(v->name);
}

static gboolean equal_func(gconstpointer a, gconstpointer b)
{
	const TypedVar *v = a;
	const TypedVar *w = b;
	return g_str_equal(v->name, w->name);
}

static void typed_var_free(gpointer data)
{
	TypedVar *v = data;
	g_free(v->name);
	g_free(v);
}

TypedVar *typed_var_new(const gchar *name, GebrGeoXmlParameterType type)
{
	TypedVar *v = g_new(TypedVar, 1);
	v->name = g_strdup(name);
	v->type = type;
	return v;
}

/* }}} */

/* GebrIExpr interface methods implementation {{{ */
static gboolean gebr_string_expr_set_var(GebrIExpr              *iface,
					 const gchar            *name,
					 GebrGeoXmlParameterType type,
					 const gchar            *value,
					 GError                **err)
{
	GebrStringExpr *self = GEBR_STRING_EXPR(iface);

	switch (type)
	{
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
		g_hash_table_insert(self->priv->vars,
				    typed_var_new(name, type),
				    g_strdup(value));
		return TRUE;
	default:
		g_set_error(err, GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_INVAL_TYPE,
			    _("Invalid variable type"));
		return FALSE;
	}
}

static gboolean gebr_string_expr_is_valid(GebrIExpr   *iface,
					  const gchar *expr,
					  GError     **err)
{
	return gebr_string_expr_eval(GEBR_STRING_EXPR(iface), expr, NULL, err);
}

static void gebr_string_expr_interface_init(GebrIExprInterface *iface)
{
	iface->set_var = gebr_string_expr_set_var;
	iface->is_valid = gebr_string_expr_is_valid;
}

/* }}} */

GebrStringExpr *gebr_string_expr_new(void)
{
	return g_object_new(GEBR_TYPE_STRING_EXPR, NULL);
}

gboolean gebr_string_expr_eval(GebrStringExpr   *self,
			       const gchar      *expr,
			       gchar           **result,
			       GError          **err)
{
	gint j;
	gint i = 0;
	gchar *name;
	gchar *value;
	TypedVar *typed_var;
	gboolean retval = TRUE;
	gboolean fetch_var = FALSE;
	GString *decoded = g_string_new(NULL);

	name = g_new(gchar, strlen(expr));

	while (expr[i])
	{
		if (expr[i] == '[' && expr[i+1] == '[') {
			i += 2;
			g_string_append_c(decoded, '[');
			continue;
		}

		if (expr[i] == '[') {
			fetch_var = TRUE;
			i++;
			j = 0;
			continue;
		}

		if (fetch_var) {
			if (expr[i] == ']') {
				fetch_var = FALSE;
				name[j] = '\0';
				if (g_hash_table_lookup_extended(self->priv->vars, name,
								 (gpointer)&typed_var,
								 (gpointer)&value))
				{
					switch (typed_var->type) {
					case GEBR_GEOXML_PARAMETER_TYPE_STRING:
					case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
					case GEBR_GEOXML_PARAMETER_TYPE_INT:
						g_string_append(decoded, value);
						break;
					default:
						g_warn_if_reached();
					}
				} else {
					retval = FALSE;
					g_set_error(err, GEBR_IEXPR_ERROR,
						    GEBR_IEXPR_ERROR_UNDEF_VAR,
						    _("Variable %s is undefined"),
						    name);
					goto exception;
				}
				i++;
				continue;
			} else {
				name[j++] = expr[i++];
				continue;
			}
		}

		if (expr[i] == ']') {
		       if (expr[i+1] != ']') {
			       retval = FALSE;
			       g_set_error(err, GEBR_IEXPR_ERROR,
					   GEBR_IEXPR_ERROR_SYNTAX,
					   _("Syntax error: unmatched closing bracket"));
			       goto exception;
		       } else {
			       g_string_append_c(decoded, ']');
			       i += 2;
		       }
		       continue;
		}

		// Escape double-quotes and backslashes
		if (expr[i] == '"' || expr[i] == '\\')
			g_string_append_c(decoded, '\\');
		g_string_append_c(decoded, expr[i++]);
	}

	if (fetch_var) {
		retval = FALSE;
		g_set_error(err, GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_SYNTAX,
			    _("Syntax error: unmatched opening bracket"));
		goto exception;
	}

	if (result)
		*result = g_strdup(decoded->str);

exception:
	g_string_free(decoded, TRUE);
	g_free(name);

	return retval;
}
