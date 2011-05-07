/*
 * struct.c
 *
 * BabelTrace - Structure Type Converter
 *
 * Copyright 2010, 2011 - Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#include <babeltrace/compiler.h>
#include <babeltrace/format.h>

#ifndef max
#define max(a, b)	((a) < (b) ? (b) : (a))
#endif

static
struct definition *_struct_definition_new(struct declaration *declaration,
				struct definition_scope *parent_scope,
				GQuark field_name, int index);
static
void _struct_definition_free(struct definition *definition);

int struct_rw(struct stream_pos *ppos, struct definition *definition)
{
	struct definition_struct *struct_definition =
		container_of(definition, struct definition_struct, p);
	unsigned long i;
	int ret;

	for (i = 0; i < struct_definition->fields->len; i++) {
		struct field *field = &g_array_index(struct_definition->fields,
						     struct field, i);
		ret = generic_rw(ppos, field->definition);
		if (ret)
			return ret;
	}
	return 0;
}

static
void _struct_declaration_free(struct declaration *declaration)
{
	struct declaration_struct *struct_declaration =
		container_of(declaration, struct declaration_struct, p);
	unsigned long i;

	free_declaration_scope(struct_declaration->scope);
	g_hash_table_destroy(struct_declaration->fields_by_name);

	for (i = 0; i < struct_declaration->fields->len; i++) {
		struct declaration_field *declaration_field =
			&g_array_index(struct_declaration->fields,
				       struct declaration_field, i);
		declaration_unref(declaration_field->declaration);
	}
	g_array_free(struct_declaration->fields, true);
	g_free(struct_declaration);
}

struct declaration_struct *
	struct_declaration_new(struct declaration_scope *parent_scope)
{
	struct declaration_struct *struct_declaration;
	struct declaration *declaration;

	struct_declaration = g_new(struct declaration_struct, 1);
	declaration = &struct_declaration->p;
	struct_declaration->fields_by_name = g_hash_table_new(g_direct_hash,
						       g_direct_equal);
	struct_declaration->fields = g_array_sized_new(FALSE, TRUE,
						sizeof(struct declaration_field),
						DEFAULT_NR_STRUCT_FIELDS);
	struct_declaration->scope = new_declaration_scope(parent_scope);
	declaration->id = CTF_TYPE_STRUCT;
	declaration->alignment = 1;
	declaration->declaration_free = _struct_declaration_free;
	declaration->definition_new = _struct_definition_new;
	declaration->definition_free = _struct_definition_free;
	declaration->ref = 1;
	return struct_declaration;
}

static
struct definition *
	_struct_definition_new(struct declaration *declaration,
			       struct definition_scope *parent_scope,
			       GQuark field_name, int index)
{
	struct declaration_struct *struct_declaration =
		container_of(declaration, struct declaration_struct, p);
	struct definition_struct *_struct;
	unsigned long i;
	int ret;

	_struct = g_new(struct definition_struct, 1);
	declaration_ref(&struct_declaration->p);
	_struct->p.declaration = declaration;
	_struct->declaration = struct_declaration;
	_struct->p.ref = 1;
	_struct->p.index = index;
	_struct->scope = new_definition_scope(parent_scope, field_name);
	_struct->fields = g_array_sized_new(FALSE, TRUE,
					    sizeof(struct field),
					    DEFAULT_NR_STRUCT_FIELDS);
	g_array_set_size(_struct->fields, struct_declaration->fields->len);
	for (i = 0; i < struct_declaration->fields->len; i++) {
		struct declaration_field *declaration_field =
			&g_array_index(struct_declaration->fields,
				       struct declaration_field, i);
		struct field *field = &g_array_index(_struct->fields,
						     struct field, i);

		field->name = declaration_field->name;
		field->definition =
			declaration_field->declaration->definition_new(declaration_field->declaration,
							  _struct->scope,
							  field->name, i);
		ret = register_field_definition(field->name,
						field->definition,
						_struct->scope);
		assert(!ret);
	}
	return &_struct->p;
}

static
void _struct_definition_free(struct definition *definition)
{
	struct definition_struct *_struct =
		container_of(definition, struct definition_struct, p);
	unsigned long i;

	assert(_struct->fields->len == _struct->declaration->fields->len);
	for (i = 0; i < _struct->fields->len; i++) {
		struct field *field = &g_array_index(_struct->fields,
						     struct field, i);
		definition_unref(field->definition);
	}
	free_definition_scope(_struct->scope);
	declaration_unref(_struct->p.declaration);
	g_free(_struct);
}

void struct_declaration_add_field(struct declaration_struct *struct_declaration,
			   const char *field_name,
			   struct declaration *field_declaration)
{
	struct declaration_field *field;
	unsigned long index;

	g_array_set_size(struct_declaration->fields, struct_declaration->fields->len + 1);
	index = struct_declaration->fields->len - 1;	/* last field (new) */
	field = &g_array_index(struct_declaration->fields, struct declaration_field, index);
	field->name = g_quark_from_string(field_name);
	declaration_ref(field_declaration);
	field->declaration = field_declaration;
	/* Keep index in hash rather than pointer, because array can relocate */
	g_hash_table_insert(struct_declaration->fields_by_name,
			    (gpointer) (unsigned long) field->name,
			    (gpointer) index);
	/*
	 * Alignment of structure is the max alignment of declarations contained
	 * therein.
	 */
	struct_declaration->p.alignment = max(struct_declaration->p.alignment,
				       field_declaration->alignment);
}

/*
 * struct_declaration_lookup_field_index - returns field index
 *
 * Returns the index of a field in a structure, or -1 if it does not
 * exist.
 */
int struct_declaration_lookup_field_index(struct declaration_struct *struct_declaration,
				       GQuark field_name)
{
	gpointer index;
	gboolean found;

	found = g_hash_table_lookup_extended(struct_declaration->fields_by_name,
				    (gconstpointer) (unsigned long) field_name,
				    NULL, &index);
	if (!found)
		return -1;
	return (int) (unsigned long) index;
}

/*
 * field returned only valid as long as the field structure is not appended to.
 */
struct declaration_field *
	struct_declaration_get_field_from_index(struct declaration_struct *struct_declaration,
					 int index)
{
	if (index < 0)
		return NULL;
	return &g_array_index(struct_declaration->fields, struct declaration_field, index);
}

/*
 * field returned only valid as long as the field structure is not appended to.
 */
struct field *
struct_definition_get_field_from_index(struct definition_struct *_struct,
					int index)
{
	if (index < 0)
		return NULL;
	return &g_array_index(_struct->fields, struct field, index);
}
