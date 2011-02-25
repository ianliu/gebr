/*   GeBR Daemon - Process and control execution of flows
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/comm/gebr-comm-protocol.h>

#include "gebrd.h"
#include "gebrd-server.h"
#include "gebrd-client.h"
#include "gebrd-job-queue.h"

/* GOBJECT STUFF */
enum {
	PROP_0,
	PROP_FS_NICKNAME,
	LAST_PROPERTY
};
//static guint property_member_offset [] = {0,
//};
enum {
	LAST_SIGNAL
};
//static guint object_signals[LAST_SIGNAL];
G_DEFINE_TYPE(GebrdUser, gebrd_user, G_TYPE_OBJECT)
static void gebrd_user_init(GebrdUser * self)
{
	self->fs_nickname = g_string_new("");
	self->queues = g_hash_table_new_full((GHashFunc)g_str_hash, (GEqualFunc)g_str_equal,
					     (GDestroyNotify)g_free, NULL);
}
static void gebrd_user_finalize(GObject * object)
{
	GebrdUser *self = (GebrdUser *) object;

	g_string_free(self->fs_nickname, TRUE);
	/* jobs */
	g_list_foreach(self->jobs, (GFunc) job_free, NULL);
	g_list_free(self->jobs);

	G_OBJECT_CLASS(gebrd_user_parent_class)->finalize(object);
}
static void
gebrd_user_set_property(GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
	GebrdUser *self = (GebrdUser *) object;
	switch (property_id) {
	case PROP_FS_NICKNAME:
		g_string_assign(self->fs_nickname, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
	gebrd_user_data_save();
}
static void
gebrd_user_get_property(GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
	GebrdUser *self = (GebrdUser *) object;
	switch (property_id) {
	case PROP_FS_NICKNAME:
		g_value_set_string(value, self->fs_nickname->str);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}
static void gebrd_user_class_init(GebrdUserClass * klass)
{ 
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebrd_user_finalize;
	
	/* properties */
	GParamSpec *spec;
	gobject_class->set_property = gebrd_user_set_property;
	gobject_class->get_property = gebrd_user_get_property;
	spec =	g_param_spec_string("fs-nickname", "", "", "", (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE)); 
	g_object_class_install_property(gobject_class, PROP_FS_NICKNAME, spec);
}

