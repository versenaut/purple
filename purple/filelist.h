/*
 * Create an opaque data type that holds the names of the files in a given directory.
 * Needed since there is no platform-agnostic API for traversing the contents of a dir.
*/

typedef struct FileList FileList;

extern FileList *	filelist_new(const char *path, const char *suffix);
extern size_t		filelist_size(const FileList *fl);
extern const char *	filelist_filename(const FileList *fl, int index);
extern const char *	filelist_filename_full(const FileList *fl, int index);
extern void		filelist_destroy(FileList *fl);
