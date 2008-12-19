/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#include "sequence.h"
#include "xml.h"
#include "types.h"
#include "error.h"
#include "parameters.h"
#include "parameters_p.h"

/*
 * internal structures and funcionts
 */

struct geoxml_sequence {
	GdomeElement * element;
};

/**
 * \internal
 * Check \p sequence is a parameter
 */
static inline gboolean
__geoxml_sequence_is_parameter(GeoXmlSequence * sequence)
{
	return (gboolean)!strcmp(gdome_el_nodeName((GdomeElement*)sequence, &exception)->str, "parameter");
}

/**
 * \internal
 * Check if \p sequence is really a sequence and can be modified (the case of parameter in a group)
 */
static int
__geoxml_sequence_check(GeoXmlSequence * sequence)
{
	if (sequence == NULL)
		return GEOXML_RETV_NULL_PTR;

	GdomeDOMString *	name;

	name = gdome_el_nodeName((GdomeElement*)sequence, &exception);
	if (!strcmp(name->str, "parameter")) {
		GeoXmlParameters *	parameters;

		parameters = (GeoXmlParameters*)gdome_el_parentNode((GdomeElement*)sequence, &exception);
		if (__geoxml_parameters_group_check(parameters) == FALSE)
			return GEOXML_RETV_NOT_MASTER_INSTANCE;

		return GEOXML_RETV_SUCCESS;
	}
	/* on success, return 0 = GEOXML_RETV_SUCCESS */
	return strcmp(name->str, "value") &&
		strcmp(name->str, "default") &&
		strcmp(name->str, "option") &&
		(strcmp(name->str, "parameters") &&
			geoxml_parameters_get_is_in_group((GeoXmlParameters*)sequence)) &&
		strcmp(name->str, "program") &&
		strcmp(name->str, "category") &&
		strcmp(name->str, "revision") &&
		strcmp(name->str, "flow") &&
		strcmp(name->str, "path") &&
		strcmp(name->str, "line");
}

/**
 * \internal
 * Check if \p sequence and \p other are exactly of the same sequence type
 */
static gboolean
__geoxml_sequence_is_same_sequence(GeoXmlSequence * sequence, GeoXmlSequence * other)
{
	return (gboolean)gdome_str_equal(
		gdome_el_nodeName((GdomeElement*)sequence, &exception),
		gdome_el_nodeName((GdomeElement*)other, &exception));
}

void
__geoxml_sequence_move_in_group(GeoXmlSequence * sequence, GeoXmlSequence * position,
	int (*move_function)(GeoXmlSequence * sequence, GeoXmlSequence * position))
{
	if (!__geoxml_sequence_is_parameter(position))
		return;

	GSList *	sequence_refs;
	GSList *	i;

	sequence_refs = __geoxml_get_elements_by_idref((GdomeElement*)sequence,
		__geoxml_get_attr_value((GdomeElement*)sequence, "id"));
	if (geoxml_parameter_get_is_in_group((GeoXmlParameter*)position)) {
		GSList *	position_refs;
		GSList *	j;

		position_refs = __geoxml_get_elements_by_idref((GdomeElement*)position,
			__geoxml_get_attr_value((GdomeElement*)position, "id"));
		for (i = g_slist_last(sequence_refs), j = g_slist_last(position_refs);
		i != NULL; i = g_slist_next(i), j = g_slist_next(j))
			move_function((GeoXmlSequence*)i->data, (GeoXmlSequence*)j->data);

		g_slist_free(position_refs);
	} else
		for (i = g_slist_last(sequence_refs); i != NULL; i = g_slist_next(i))
			gdome_n_removeChild(gdome_el_parentNode((GdomeElement*)i->data, &exception),
				(GdomeNode*)i->data, &exception);
	g_slist_free(sequence_refs);
}

int
__geoxml_sequence_previous(GeoXmlSequence ** sequence)
{
	*sequence = (GeoXmlSequence*)__geoxml_previous_same_element((GdomeElement*)*sequence);

	return (*sequence != NULL) ? GEOXML_RETV_SUCCESS : GEOXML_RETV_INVALID_INDEX;
}

int
__geoxml_sequence_next(GeoXmlSequence ** sequence)
{
	*sequence = (GeoXmlSequence*)__geoxml_next_same_element((GdomeElement*)*sequence);

	return (*sequence != NULL) ? GEOXML_RETV_SUCCESS : GEOXML_RETV_INVALID_INDEX;
}

int
__geoxml_sequence_remove(GeoXmlSequence * sequence)
{
	gdome_n_removeChild(gdome_el_parentNode((GdomeElement*)sequence, &exception),
		(GdomeNode*)sequence, &exception);

	return GEOXML_RETV_SUCCESS;
}

GeoXmlSequence *
__geoxml_sequence_append_clone(GeoXmlSequence * sequence)
{
	GeoXmlSequence *	clone;
	GeoXmlSequence *	after_last;

	clone = (GeoXmlSequence *)gdome_n_cloneNode((GdomeNode*)sequence, TRUE, &exception);
	__geoxml_element_reassign_ids((GdomeElement*)clone);

	after_last = sequence;
	while (!__geoxml_sequence_next(&after_last));
	gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
		(GdomeNode*)clone, (GdomeNode*)after_last, &exception);

	return clone;
}

int
__geoxml_sequence_move_before(GeoXmlSequence * sequence, GeoXmlSequence * position)
{
	if (position != NULL && !__geoxml_sequence_is_same_sequence(sequence, position))
		return GEOXML_RETV_DIFFERENT_SEQUENCES;

	gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
		(GdomeNode*)sequence, (GdomeNode*)position, &exception);

	return exception == GDOME_NOEXCEPTION_ERR
		? GEOXML_RETV_SUCCESS : GEOXML_RETV_DIFFERENT_SEQUENCES;
}

int
__geoxml_sequence_move_after(GeoXmlSequence * sequence, GeoXmlSequence * position)
{
	if (position == NULL) {
		GdomeNode *	parent_element;

		parent_element = gdome_el_parentNode((GdomeElement*)sequence, &exception);
		gdome_n_insertBefore(parent_element, (GdomeNode*)sequence,
			gdome_n_firstChild(parent_element, &exception),
			&exception);

		return exception == GDOME_NOEXCEPTION_ERR
			? GEOXML_RETV_SUCCESS : GEOXML_RETV_DIFFERENT_SEQUENCES;
	} else {
		__geoxml_sequence_next(&position);
		gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
			(GdomeNode*)sequence, (GdomeNode*)position, &exception);

		return exception == GDOME_NOEXCEPTION_ERR
			? GEOXML_RETV_SUCCESS : GEOXML_RETV_DIFFERENT_SEQUENCES;
	}
}

int
__geoxml_sequence_move_up(GeoXmlSequence * sequence)
{
	GeoXmlSequence *	previous;

	previous = sequence;
	geoxml_sequence_previous(&previous);
	if (previous == NULL)
		return GEOXML_RETV_INVALID_INDEX;

	gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
		(GdomeNode*)sequence, (GdomeNode*)previous, &exception);

	return GEOXML_RETV_SUCCESS;
}

int
__geoxml_sequence_move_down(GeoXmlSequence * sequence)
{
	GeoXmlSequence *	next;

	next = sequence;
	geoxml_sequence_next(&next);
	if (next == NULL)
		return GEOXML_RETV_INVALID_INDEX;
	next = (GeoXmlSequence*)__geoxml_next_element((GdomeElement*)next);

	gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
			     (GdomeNode*)sequence, (GdomeNode*)next, &exception);

	return GEOXML_RETV_SUCCESS;
}

/*
 * library functions.
 */

int
geoxml_sequence_previous(GeoXmlSequence ** sequence)
{
	int ret;
	if ((ret = __geoxml_sequence_check(*sequence))) {
		*sequence = NULL;
		return ret;
	}

	return __geoxml_sequence_previous(sequence);
}

int
geoxml_sequence_next(GeoXmlSequence ** sequence)
{
	int ret;
	if ((ret = __geoxml_sequence_check(*sequence))) {
		*sequence = NULL;
		return ret;
	}

	return __geoxml_sequence_next(sequence);
}

GeoXmlSequence *
geoxml_sequence_append_clone(GeoXmlSequence * sequence)
{
	int			ret;
	GeoXmlSequence *	clone;

	if ((ret = __geoxml_sequence_check(sequence)))
		return NULL;
	clone = __geoxml_sequence_append_clone(sequence);
	/* append reference to clone for others group instances */
	if (__geoxml_sequence_is_parameter(sequence) && geoxml_parameter_get_is_in_group((GeoXmlParameter*)sequence)) {
		GdomeElement *	reference_element;

		__geoxml_foreach_element(reference_element,
		__geoxml_get_elements_by_idref((GdomeElement*)sequence, __geoxml_get_attr_value((GdomeElement*)sequence, "id")))
			__geoxml_sequence_append_clone((GeoXmlSequence*)gdome_el_parentNode(reference_element, &exception));
	}

	return clone;
}

gint
geoxml_sequence_get_index(GeoXmlSequence * sequence)
{
	gint index;

	for (index = -1; sequence != NULL; geoxml_sequence_previous(&sequence))
		index++;

	return index;
}

//TODO: improve performance
GeoXmlSequence *
geoxml_sequence_get_at(GeoXmlSequence * sequence, gulong index)
{
	if (sequence == NULL)
		return NULL;

	gulong	i;

	i = geoxml_sequence_get_index(sequence);
	if (i == index)
		return sequence;
	if (i < index)
		for (; i < index && sequence != NULL; ++i, geoxml_sequence_next(&sequence));
	else
		for (; i > index && sequence != NULL; --i, geoxml_sequence_previous(&sequence));

	return sequence;
}

int
geoxml_sequence_remove(GeoXmlSequence * sequence)
{
	int ret;

	if ((ret = __geoxml_sequence_check(sequence)))
		return ret;
	if ((ret = __geoxml_sequence_remove(sequence)))
		return ret;
	if (__geoxml_sequence_is_parameter(sequence) && geoxml_parameter_get_is_in_group((GeoXmlParameter*)sequence)) {
		GdomeElement *	reference_element;

		__geoxml_foreach_element(reference_element,
		__geoxml_get_elements_by_idref((GdomeElement*)sequence, __geoxml_get_attr_value((GdomeElement*)sequence, "id")))
			__geoxml_sequence_remove((GeoXmlSequence*)gdome_el_parentNode(reference_element, &exception));
	}

	return GEOXML_RETV_SUCCESS;
}

int
geoxml_sequence_move_before(GeoXmlSequence * sequence, GeoXmlSequence * position)
{
	int ret;

	if ((ret = __geoxml_sequence_check(sequence)))
		return ret;
	if (__geoxml_sequence_is_parameter(sequence) && geoxml_parameter_get_is_in_group((GeoXmlParameter*)sequence))
		__geoxml_sequence_move_in_group(sequence, position, __geoxml_sequence_move_before);

	return __geoxml_sequence_move_before(sequence, position);
}

int
geoxml_sequence_move_after(GeoXmlSequence * sequence, GeoXmlSequence * position)
{
	int ret;

	if ((ret = __geoxml_sequence_check(sequence)))
		return ret;
	if (__geoxml_sequence_is_parameter(sequence) && geoxml_parameter_get_is_in_group((GeoXmlParameter*)sequence))
		__geoxml_sequence_move_in_group(sequence, position, __geoxml_sequence_move_after);

	return __geoxml_sequence_move_after(sequence, position);
}

int
geoxml_sequence_move_up(GeoXmlSequence * sequence)
{
	int ret;

	if ((ret = __geoxml_sequence_move_up(sequence)))
		return ret;
	if (__geoxml_sequence_is_parameter(sequence) && geoxml_parameter_get_is_in_group((GeoXmlParameter*)sequence)) {
		GdomeElement *	reference_element;

		__geoxml_foreach_element(reference_element,
		__geoxml_get_elements_by_idref((GdomeElement*)sequence, __geoxml_get_attr_value((GdomeElement*)sequence, "id")))
			__geoxml_sequence_move_up((GeoXmlSequence*)gdome_el_parentNode(reference_element, &exception));
	}
	return GEOXML_RETV_SUCCESS;
}

int
geoxml_sequence_move_down(GeoXmlSequence * sequence)
{
	int ret;

	if ((ret = __geoxml_sequence_move_down(sequence)))
		return ret;
	if (__geoxml_sequence_is_parameter(sequence) && geoxml_parameter_get_is_in_group((GeoXmlParameter*)sequence)) {
		GdomeElement *	reference_element;

		__geoxml_foreach_element(reference_element,
		__geoxml_get_elements_by_idref((GdomeElement*)sequence, __geoxml_get_attr_value((GdomeElement*)sequence, "id")))
			__geoxml_sequence_move_down((GeoXmlSequence*)gdome_el_parentNode(reference_element, &exception));
	}
	return GEOXML_RETV_SUCCESS;
}
