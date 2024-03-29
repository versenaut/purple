/*
 * iter.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Node-internal calls to implement iter support. See purple.h and api-iter.c, too.
*/

#include <stddef.h>

extern void	iter_init_dynarr(PIter *iter, const DynArr *arr);
extern void	iter_init_dynarr_string(PIter *iter, const DynArr *arr, size_t offset);
extern void	iter_init_dynarr_enum_negative(PIter *iter, const DynArr *arr, size_t offset);
extern void	iter_init_dynarr_uint16_ffff(PIter *iter, const DynArr *arr, size_t offset);
extern void	iter_init_list(PIter *iter, List *list);
