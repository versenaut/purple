/*
 * iter.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Node-internal calls to implement iter supprt. See purple.h and api-iter.c, too.
*/

#include <stddef.h>

extern void	iter_init_dynarr(PIter *iter, DynArr *arr);
extern void	iter_init_list(PIter *iter, List *list);
extern void	iter_set_string_mode(PIter *iter, size_t offset);
