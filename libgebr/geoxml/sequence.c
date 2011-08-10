/*   libgebr - GeBR Library
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

#include <string.h>

#include <gdome.h>

#include "error.h"
#include "parameter.h"
#include "parameter_group.h"
#include "parameter_p.h"
#include "parameters.h"
#include "parameters_p.h"
#include "sequence.h"
#include "types.h"
#include "xml.h"
#include "object.h"

/*
 * internal structures and funcionts
 */

struct gebr_geoxml_sequence {
	GdomeElement *element;
};

/**
 * \internal
 * Check \p sequence is a parameter
 */
static inline gboolean __gebr_geoxml_sequence_is_parameter(GebrGeoXmlSequence * sequence)
{
	GdomeDOMString *string = gdome_el_nodeName((GdomeElement *) sequence, &exception);
	gboolean retval = !strcmp(string->str, "parameter");
	gdome_str_unref(string);
	return retval;
}

/**
 * \internal
 * Check if \p sequence is really a sequence and can be modified (the case of parameter in a group)
 */
static int __gebr_geoxml_sequence_check(GebrGeoXmlSequence * sequence, gboolean check_master_instance)
{
	int retval;

	if (sequence == NULL)
		return GEBR_GEOXML_RETV_NULL_PTR;

	GdomeDOMString *tag = gdome_el_tagName((GdomeElement *) sequence, &exception);

	/* parameter in group check */
	if (check_master_instance && strcmp(tag->str, "parameter") == 0) {
		GebrGeoXmlParameters *parameters;

		parameters = (GebrGeoXmlParameters *) gdome_el_parentNode((GdomeElement *) sequence, &exception);
		if (__gebr_geoxml_parameters_group_check(parameters) == FALSE) {
			retval = GEBR_GEOXML_RETV_NOT_MASTER_INSTANCE;
			gdome_el_unref((GdomeElement*)parameters, &exception);
			goto out;
		}
		gdome_el_unref((GdomeElement*)parameters, &exception);
		retval = GEBR_GEOXML_RETV_SUCCESS;
		goto out;
	}

	/* on success, return 0 = GEBR_GEOXML_RETV_SUCCESS */
	retval = strcmp(tag->str, "server")
		&& strcmp(tag->str, "value")
		&& strcmp(tag->str, "default")
		&& strcmp(tag->str, "option")
		&& strcmp(tag->str, "program")
		&& strcmp(tag->str, "parameter")
		&& strcmp(tag->str, "parameters")
		&& strcmp(tag->str, "category")
		&& strcmp(tag->str, "revision")
		&& strcmp(tag->str, "flow")
		&& strcmp(tag->str, "path")
		&& strcmp(tag->str, "line");

out:
	gdome_str_unref(tag);
	return retval;
}

int
__gebr_geoxml_sequence_move_after_before(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * position,
					 int (*move_function) (GebrGeoXmlSequence * sequence,
							       GebrGeoXmlSequence * position))
{
	gint ret;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlParameterGroup *group;

	if (!__gebr_geoxml_sequence_is_parameter(sequence))
		return move_function(sequence, position);

	parameter = GEBR_GEOXML_PARAMETER(sequence);
	group = gebr_geoxml_parameter_get_group(parameter);

	if (group) {
		GSList *i;
		GSList *sequence_refs;

		sequence_refs = __gebr_geoxml_parameter_get_referencee_list(parameter);
		ret = move_function(sequence, position);

		if (ret != GEBR_GEOXML_RETV_SUCCESS)
			return ret;

		if (group == gebr_geoxml_parameter_get_group(GEBR_GEOXML_PARAMETER(position))) {
			GSList *j;
			GSList *position_refs;

			position_refs = __gebr_geoxml_parameter_get_referencee_list(GEBR_GEOXML_PARAMETER(position));

			for (i = sequence_refs, j = position_refs;
			     i != NULL && j != NULL; i = g_slist_next(i), j = g_slist_next(j))
				move_function((GebrGeoXmlSequence *) i->data, (GebrGeoXmlSequence *) j->data);

			g_slist_free(position_refs);
		} else {
			for (i = sequence_refs; i != NULL; i = g_slist_next(i))
				gdome_n_removeChild(gdome_el_parentNode((GdomeElement *) i->data, &exception),
						    (GdomeNode *) i->data, &exception);

			__gebr_geoxml_parameters_do_insert_in_group_stuff(gebr_geoxml_parameter_get_parameters
									  (GEBR_GEOXML_PARAMETER(sequence)),
									  GEBR_GEOXML_PARAMETER(sequence));
		}

		g_slist_free(sequence_refs);
	} else {
		if ((ret = move_function(sequence, position)))
			return ret;

		if (gebr_geoxml_parameter_get_group((GebrGeoXmlParameter *) position) != NULL)
			__gebr_geoxml_parameters_do_insert_in_group_stuff(gebr_geoxml_parameter_get_parameters
									  (GEBR_GEOXML_PARAMETER(sequence)),
									  GEBR_GEOXML_PARAMETER(sequence));
	}

	return ret;
}

int __gebr_geoxml_sequence_previous(GebrGeoXmlSequence ** sequence)
{
	int ret = __gebr_geoxml_sequence_check(*sequence, FALSE);

	if (ret) {
		*sequence = NULL;
		return ret;
	}

	GdomeElement *previous = __gebr_geoxml_previous_same_element((GdomeElement *) * sequence);
	gdome_el_unref((GdomeElement*) *sequence, &exception);

	if (previous) {
		*sequence = GEBR_GEOXML_SEQUENCE(previous);
		return GEBR_GEOXML_RETV_SUCCESS;
	}

	*sequence = NULL;
	return GEBR_GEOXML_RETV_INVALID_INDEX;
}

void __gebr_geoxml_sequence_remove(GebrGeoXmlSequence * sequence)
{
	GdomeNode *parent;
	parent = gdome_el_parentNode((GdomeElement *) sequence, &exception);
	gdome_n_unref(gdome_n_removeChild(parent, (GdomeNode *) sequence, &exception), &exception);
	gdome_n_unref(parent, &exception);
	gebr_geoxml_object_unref(sequence);
}

GebrGeoXmlSequence *__gebr_geoxml_sequence_append_clone(GebrGeoXmlSequence * sequence)
{
	GdomeNode *clone;
	GdomeNode *parent;
	GdomeNode *insert;

	parent = gdome_n_parentNode((GdomeNode *) sequence, &exception);
	clone = gdome_n_cloneNode((GdomeNode *) sequence, TRUE, &exception);
	insert = gdome_n_insertBefore_protected(parent, clone, NULL, &exception);
	gdome_n_unref(parent, &exception);
	gdome_n_unref(clone, &exception);

	return GEBR_GEOXML_SEQUENCE(insert);
}

int
__gebr_geoxml_sequence_move_before(GebrGeoXmlSequence *sequence,
				   GebrGeoXmlSequence *position)
{
	GdomeNode *parent;
	GdomeNode *insert;
	GdomeException exc1, exc2;

	parent = gdome_n_parentNode((GdomeNode *) position, &exc1);
	insert = gdome_n_insertBefore_protected(parent, (GdomeNode *)sequence,
						(GdomeNode *)position, &exc2);
	gdome_n_unref(parent, &exception);
	gdome_n_unref(insert, &exception);

	if (exc1 == GDOME_NOEXCEPTION_ERR || exc2 == GDOME_NOEXCEPTION_ERR)
		return GEBR_GEOXML_RETV_DIFFERENT_SEQUENCES;
	return GEBR_GEOXML_RETV_SUCCESS;
}

int __gebr_geoxml_sequence_move_after(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * position)
{
	if (position == NULL) {
		GebrGeoXmlSequence *current = NULL;
		GebrGeoXmlSequence *first;
		GebrGeoXmlSequence *prev;

		prev = sequence;
		gebr_geoxml_object_ref(sequence);

		while (prev) {
			if (current)
				gebr_geoxml_object_unref(current);
			current = prev;
			gebr_geoxml_object_ref(current);
			__gebr_geoxml_sequence_previous(&prev);
		}

		first = current;

		if (first != sequence) {
			GdomeNode *parent = gdome_el_parentNode((GdomeElement *) sequence, &exception);
			gdome_n_unref(gdome_n_insertBefore_protected(parent,
					     (GdomeNode *) sequence, (GdomeNode *) first, &exception), &exception);
			gdome_n_unref(parent, &exception);
		} else
			exception = GDOME_NOEXCEPTION_ERR;
		gebr_geoxml_object_unref(first);
	} else if (sequence != position) {
		GdomeElement *next_element;

		next_element = __gebr_geoxml_next_element((GdomeElement *) position);
		GdomeNode *parent = gdome_el_parentNode((GdomeElement *) position, &exception);
		gdome_n_unref(gdome_n_insertBefore_protected(parent,
					       (GdomeNode *) sequence, (GdomeNode *) next_element, &exception), &exception);
		gdome_n_unref(parent, &exception);
		gdome_el_unref(next_element, &exception);
	} else
		exception = GDOME_NOEXCEPTION_ERR;

	return exception == GDOME_NOEXCEPTION_ERR ? GEBR_GEOXML_RETV_SUCCESS : GEBR_GEOXML_RETV_DIFFERENT_SEQUENCES;
}

int __gebr_geoxml_sequence_move_up(GebrGeoXmlSequence * sequence)
{
	GebrGeoXmlSequence *previous;

	previous = sequence;
	gebr_geoxml_sequence_previous(&previous);
	if (previous == NULL)
		return GEBR_GEOXML_RETV_INVALID_INDEX;

	gdome_n_insertBefore_protected(gdome_el_parentNode((GdomeElement *) sequence, &exception),
			     (GdomeNode *) sequence, (GdomeNode *) previous, &exception);

	return GEBR_GEOXML_RETV_SUCCESS;
}

int __gebr_geoxml_sequence_move_down(GebrGeoXmlSequence * sequence)
{
	GebrGeoXmlSequence *next;

	next = sequence;
	gebr_geoxml_sequence_next(&next);
	if (next == NULL)
		return GEBR_GEOXML_RETV_INVALID_INDEX;
	next = (GebrGeoXmlSequence *) __gebr_geoxml_next_element((GdomeElement *) next);

	gdome_n_insertBefore_protected(gdome_el_parentNode((GdomeElement *) sequence, &exception),
			     (GdomeNode *) sequence, (GdomeNode *) next, &exception);

	return GEBR_GEOXML_RETV_SUCCESS;
}

/*
 * library functions.
 */

int gebr_geoxml_sequence_previous(GebrGeoXmlSequence ** sequence)
{
	int ret;
	if ((ret = __gebr_geoxml_sequence_check(*sequence, FALSE))) {
		*sequence = NULL;
		return ret;
	}

	return __gebr_geoxml_sequence_previous(sequence);
}

int gebr_geoxml_sequence_next(GebrGeoXmlSequence ** sequence)
{
	int ret = __gebr_geoxml_sequence_check(*sequence, FALSE);

	if (ret) {
		*sequence = NULL;
		return ret;
	}

	GdomeElement *next = __gebr_geoxml_next_same_element((GdomeElement *) *sequence);
	gdome_el_unref((GdomeElement*) *sequence, &exception);

	if (next) {
		*sequence = GEBR_GEOXML_SEQUENCE(next);
		return GEBR_GEOXML_RETV_SUCCESS;
	}

	*sequence = NULL;
	return GEBR_GEOXML_RETV_INVALID_INDEX;
}

GebrGeoXmlSequence *gebr_geoxml_sequence_append_clone(GebrGeoXmlSequence * sequence)
{
	GebrGeoXmlSequence *clone;

	if (__gebr_geoxml_sequence_check(sequence, TRUE))
		return NULL;

	clone = __gebr_geoxml_sequence_append_clone(sequence);
	/* append reference to clone for others group instances */
	if (__gebr_geoxml_sequence_is_parameter(sequence)
	    && gebr_geoxml_parameter_get_is_in_group((GebrGeoXmlParameter *) sequence)) {
		GSList * list;

		list = __gebr_geoxml_parameter_get_referencee_list(GEBR_GEOXML_PARAMETER(sequence));

		for (GSList *i = list; i; i = i->next)
		    __gebr_geoxml_sequence_append_clone(GEBR_GEOXML_SEQUENCE(i->data));

		g_slist_free(list);
	}

	return clone;
}

gint gebr_geoxml_sequence_get_index(GebrGeoXmlSequence * sequence)
{
	gint index;
	GebrGeoXmlSequence *i = sequence;

	gebr_geoxml_object_ref(sequence);

	for (index = -1; i; __gebr_geoxml_sequence_previous(&i))
		index++;

	return index;
}

//TODO: improve performance
GebrGeoXmlSequence *gebr_geoxml_sequence_get_at(GebrGeoXmlSequence * sequence, gulong index)
{
	if (sequence == NULL)
		return NULL;

	gulong i;

	i = gebr_geoxml_sequence_get_index(sequence);
	if (i == index)
		return sequence;
	if (i < index)
		for (; i < index && sequence != NULL; ++i, gebr_geoxml_sequence_next(&sequence)) ;
	else
		for (; i > index && sequence != NULL; --i, gebr_geoxml_sequence_previous(&sequence)) ;

	return sequence;
}

int gebr_geoxml_sequence_remove(GebrGeoXmlSequence * sequence)
{
	int ret = __gebr_geoxml_sequence_check(sequence, TRUE);

	if (ret != GEBR_GEOXML_RETV_SUCCESS)
		return ret;

	GdomeDOMString *tag = gdome_el_tagName((GdomeElement *) sequence, &exception);

	/* last instance check */
	if (!strcmp(tag->str, "parameters")) {
		GebrGeoXmlParameters *parameters = (GebrGeoXmlParameters*)sequence;
		GebrGeoXmlParameterGroup *group = gebr_geoxml_parameters_get_group(parameters);
		GebrGeoXmlParameters *template = gebr_geoxml_parameter_group_get_template(group);
		if (group != NULL && (gebr_geoxml_parameter_group_get_instances_number(group) == 1 ||
		    template == parameters))
			ret = GEBR_GEOXML_RETV_INVALID_INDEX;
		else
			ret = GEBR_GEOXML_RETV_SUCCESS;
		gebr_geoxml_object_unref(group);
		gebr_geoxml_object_unref(template);
	} else if (__gebr_geoxml_sequence_is_parameter(sequence) &&
		   gebr_geoxml_parameter_get_is_in_group((GebrGeoXmlParameter *) sequence)) {
		GSList * list;

		list = __gebr_geoxml_parameter_get_referencee_list(GEBR_GEOXML_PARAMETER(sequence));
		for (GSList *i = list; i; i = i->next)
			__gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(i->data));
		g_slist_free(list);
	}
	if (ret == GEBR_GEOXML_RETV_SUCCESS)
		__gebr_geoxml_sequence_remove(sequence);

	gdome_str_unref(tag);
	return ret;
}

gboolean gebr_geoxml_sequence_is_same_sequence(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * other)
{
	gboolean is_same;
	GdomeDOMString *str1, *str2;

	if (sequence == NULL || other == NULL)
		return FALSE;

	str1 = gdome_el_nodeName((GdomeElement *) sequence, &exception);
	str2 = gdome_el_nodeName((GdomeElement *) other, &exception);
	is_same = (gboolean) gdome_str_equal(str1, str2);
	gdome_str_unref(str1);
	gdome_str_unref(str2);

	return is_same;
}

int gebr_geoxml_sequence_move_into_group(GebrGeoXmlSequence * sequence, GebrGeoXmlParameterGroup * parameter_group)
{
	int ret;
	GebrGeoXmlParameters *template;

	if (parameter_group == NULL)
		return GEBR_GEOXML_RETV_NULL_PTR;
	if ((ret = __gebr_geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if (!__gebr_geoxml_sequence_is_parameter(sequence))
		return GEBR_GEOXML_RETV_DIFFERENT_SEQUENCES;

	if (gebr_geoxml_parameter_get_is_in_group((GebrGeoXmlParameter *) sequence)) {
		GSList *referencee;
		GdomeElement *parameter_element;

		referencee = __gebr_geoxml_parameter_get_referencee_list(GEBR_GEOXML_PARAMETER(sequence));

		__gebr_geoxml_foreach_element(parameter_element, referencee)
		    __gebr_geoxml_sequence_remove((GebrGeoXmlSequence *) parameter_element);
	}

	template = gebr_geoxml_parameter_group_get_template(parameter_group);
	gdome_el_insertBefore_protected((GdomeElement *) template, (GdomeNode *) sequence, NULL, &exception);
	__gebr_geoxml_parameters_do_insert_in_group_stuff(template, GEBR_GEOXML_PARAMETER(sequence));

	return ret;
}

int gebr_geoxml_sequence_move_before(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * position)
{
	int ret;

	if ((ret = __gebr_geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if (position != NULL && !gebr_geoxml_sequence_is_same_sequence(sequence, position))
		return GEBR_GEOXML_RETV_DIFFERENT_SEQUENCES;
	return __gebr_geoxml_sequence_move_after_before(sequence, position, __gebr_geoxml_sequence_move_before);
}

int gebr_geoxml_sequence_move_after(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * position)
{
	int ret;

	if ((ret = __gebr_geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if (position != NULL && !gebr_geoxml_sequence_is_same_sequence(sequence, position))
		return GEBR_GEOXML_RETV_DIFFERENT_SEQUENCES;
	return __gebr_geoxml_sequence_move_after_before(sequence, position, __gebr_geoxml_sequence_move_after);
}

int gebr_geoxml_sequence_move_up(GebrGeoXmlSequence * sequence)
{
	int ret;

	if ((ret = __gebr_geoxml_sequence_check(sequence, TRUE)))
		return ret;

	if ((ret = __gebr_geoxml_sequence_move_up(sequence)))
		return ret;

	if (__gebr_geoxml_sequence_is_parameter(sequence)
	    && gebr_geoxml_parameter_get_is_in_group((GebrGeoXmlParameter *) sequence)) {
		GSList * list;
		GdomeElement *parameter_element;

		list = __gebr_geoxml_parameter_get_referencee_list(GEBR_GEOXML_PARAMETER(sequence));
		__gebr_geoxml_foreach_element(parameter_element, list)
		    __gebr_geoxml_sequence_move_up((GebrGeoXmlSequence *) parameter_element);
	}
	return GEBR_GEOXML_RETV_SUCCESS;
}

int gebr_geoxml_sequence_move_down(GebrGeoXmlSequence * sequence)
{
	int ret;

	if ((ret = __gebr_geoxml_sequence_check(sequence, TRUE)))
		return ret;
	if ((ret = __gebr_geoxml_sequence_move_down(sequence)))
		return ret;
	if (__gebr_geoxml_sequence_is_parameter(sequence)
	    && gebr_geoxml_parameter_get_is_in_group((GebrGeoXmlParameter *) sequence)) {
		GSList * list;
		GdomeElement *parameter_element;

		list = __gebr_geoxml_parameter_get_referencee_list(GEBR_GEOXML_PARAMETER(sequence));
		__gebr_geoxml_foreach_element(parameter_element, list)
		    __gebr_geoxml_sequence_move_down((GebrGeoXmlSequence *) parameter_element);
	}
	return GEBR_GEOXML_RETV_SUCCESS;
}
