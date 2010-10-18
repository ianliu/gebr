#include "gebr-gui-styles-combo.h"

struct _GebrGuiStylesComboPrivate {
	gchar *path;
};

//==============================================================================
// PROTOTYPES								       =
//==============================================================================
G_DEFINE_TYPE (GebrGuiStylesCombo, gebr_gui_styles_combo, GTK_TYPE_COMBO_BOX)

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================
static void gebr_gui_styles_combo_class_init (GebrGuiStylesComboClass *klass)
{
	g_type_class_add_private (klass, sizeof (GebrGuiStylesComboPrivate));
}

static void gebr_gui_styles_combo_init (GebrGuiStylesCombo *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
						  GEBR_GUI_TYPE_STYLES_COMBO,
						  GebrGuiStylesComboPrivate);

	self->priv->path = NULL;
}

//==============================================================================
// PRIVATE METHODS							       =
//==============================================================================

//==============================================================================
// PUBLIC METHODS							       =
//==============================================================================
GtkWidget *gebr_gui_styles_combo_new (const gchar *path)
{
	GtkWidget *widget;
	GebrGuiStylesCombo *self;

	widget = g_object_new (GEBR_GUI_TYPE_STYLES_COMBO, NULL);
	self = GEBR_GUI_STYLES_COMBO (widget);
	self->priv->path = g_strdup (path);

	return widget;
}

gchar *gebr_gui_styles_combo_get_selected (GebrGuiStylesCombo *self)
{
	return NULL;
}
