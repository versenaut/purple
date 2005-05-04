/*
 * purple-plugin.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
*/

#if defined __win32
#define PURPLE_PLUGIN extern __declspec(dllexport)
#else
#define PURPLE_PLUGIN extern
#endif

PURPLE_PLUGIN void	init(void);
