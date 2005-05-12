/*
 * purple-plugin.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
*/

#if defined __win32
#define PURPLE_PLUGIN __declspec(dllexport)
#else
#define PURPLE_PLUGIN
#endif

extern PURPLE_PLUGIN void	init(void);
