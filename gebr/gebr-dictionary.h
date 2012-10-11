/*
 * gebr-dictionary.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core Team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEBR_DICTIONARY_H__
#define __GEBR_DICTIONARY_H__

#include <gtk/gtk.h>
#include <libgebr/geoxml/geoxml.h>

G_BEGIN_DECLS

#define GEBR_TYPE_DICT_COMPLETE            (gebr_dict_complete_get_type ())
#define GEBR_DICT_COMPLETE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TYPE_DICT_COMPLETE, GebrDictComplete))
#define GEBR_DICT_COMPLETE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEBR_TYPE_DICT_COMPLETE, GebrDictCompleteClass))
#define GEBR_IS_DICT_COMPLETE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TYPE_DICT_COMPLETE))
#define GEBR_IS_DICT_COMPLETE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEBR_TYPE_DICT_COMPLETE))
#define GEBR_DICT_COMPLETE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEBR_TYPE_DICT_COMPLETE, GebrDictCompleteClass))

typedef struct _GebrDictComplete GebrDictComplete;
typedef struct _GebrDictCompleteClass GebrDictCompleteClass;
typedef struct _GebrDictCompletePriv GebrDictCompletePriv;

struct _GebrDictComplete
{
	GObject parent;
	GebrDictCompletePriv *priv;
};

struct _GebrDictCompleteClass {
	GObjectClass parent_class;
};

GType gebr_dict_complete_get_type(void);

GebrDictComplete *gebr_dict_complete_new(void);

void gebr_dict_complete_set_documents(GebrDictComplete *self,
				      GebrGeoXmlDocument *proj,
				      GebrGeoXmlDocument *line,
				      GebrGeoXmlDocument *flow);

G_END_DECLS

#endif /* end of include guard: __GEBR_DICTIONARY_H__ */

