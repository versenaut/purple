/*
 * filelist.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * A module to list files in a given directory. Handy for plug-in discovery and loading.
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined _WIN32
# include <windows.h>
# define DIR_SEPARATOR	"\\"
# define PATH_MAX	MAX_PATH
#else
# include <sys/types.h>
# include <dirent.h>
# define DIR_SEPARATOR	"/"
#endif

#include "dynarr.h"
#include "log.h"
#include "mem.h"
#include "strutil.h"

#include "filelist.h"

struct FileList
{
	char	path[PATH_MAX];
	DynArr	*files;
};

/* ----------------------------------------------------------------------------------------- */

/* A filename  comparison callback. */
static int cb_compare(const void *a, const void *b)
{
	char	*sa = *(char **) a, *sb = *(char **) b;

	return strcmp(sa, sb);
}

/* Sort the dynamic array of files in alphabetical order. */
static void sort_list(FileList *fl)
{
	if(fl->files != NULL)
		dynarr_sort(fl->files, cb_compare);
}

/* ----------------------------------------------------------------------------------------- */

#if defined _WIN32

FileList * filelist_new(const char *path, const char *suffix)
{
	FileList	*fl;
	char		pat[PATH_MAX * 2], *name;
	HANDLE		find;
	WIN32_FIND_DATA	data;

	fl = mem_alloc(sizeof *fl);
	if(fl == NULL)
		return NULL;
	stu_strncpy(fl->path, sizeof fl->path, path);
	fl->files = NULL;

	sprintf(pat, "%s\\*%s", path, suffix);
	find = FindFirstFile(pat, &data);
	if(find == INVALID_HANDLE_VALUE)
	{
		mem_free(fl);
		return NULL;
	}
	while(1)
	{
		if((name = stu_strdup(data.cFileName)) != NULL)
		{
			if(fl->files == NULL)
				fl->files = dynarr_new(sizeof name, 16);
			dynarr_append(fl->files, &name, NULL);
		}
		if(!FindNextFile(find, &data))
			break;
	}
	FindClose(find);
	sort_list(fl);

	return fl;
}

/* ----------------------------------------------------------------------------------------- */

#else	/* ! __win32 */

static int has_suffix(const char *name, const char *suffix)
{
	const char	*p;

	if(name == NULL)
		return 0;
	if(suffix == NULL)
		return 1;
	if((p = strrchr(name, *suffix)) != NULL)
		return strcmp(p, suffix) == 0;
	return 0;
}

FileList * filelist_new(const char *path, const char *suffix)
{
	FileList	*fl;

	fl = mem_alloc(sizeof *fl);
	if(fl != NULL)
	{
		DIR	*dir;

		if(suffix && *suffix == '*')
			LOG_WARN(("Suffix is not glob expression, but a pure string"));

		stu_strncpy(fl->path, sizeof fl->path, path);
		fl->files = NULL;
		if((dir = opendir(path)) != NULL)
		{
			struct dirent	*de;
			const char	*name;

			while((de = readdir(dir)) != NULL)
			{
				if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
					continue;
				if(!has_suffix(de->d_name, suffix))
					continue;
				name = stu_strdup(de->d_name);
				if(name != NULL)
				{
					if(fl->files == NULL)
						fl->files = dynarr_new(sizeof name, 16);
					dynarr_append(fl->files, &name, NULL);
				}
			}
			closedir(dir);
			sort_list(fl);
		}
	}
	return fl;
}

#endif

/* ----------------------------------------------------------------------------------------- */

/* Platform-neutral routines. All the magic is done in filelist_new(). */

size_t filelist_size(const FileList *fl)
{
	if(fl != NULL && fl->files != NULL)
		return dynarr_size(fl->files);
	return 0;
}

const char * filelist_filename(const FileList *fl, int index)
{
	void	*data;

	if(fl == NULL || index < 0)
		return NULL;
	if((data = dynarr_index(fl->files, index)) != NULL)
		return *(const char **) data;
	return NULL;
}

const char * filelist_filename_full(const FileList *fl, int index)
{
	const char	*name;

	if((name = filelist_filename(fl, index)) != NULL)
	{
		static char	buf[PATH_MAX];

		snprintf(buf, sizeof buf, "%s" DIR_SEPARATOR "%s", fl->path, name);
		return buf;
	}
	return NULL;
}

void filelist_destroy(FileList *fl)
{
	if(fl == NULL)
		return;
	if(fl->files != NULL)
	{
		unsigned int	i;

		for(i = 0; i < dynarr_size(fl->files); i++)
			mem_free(*(char **) dynarr_index(fl->files, i));
		dynarr_destroy(fl->files);
	}
	mem_free(fl);
}
