/*
 * Create an opaque data type that holds the names of the files in a given directory.
 * Needed since there is no platform-agnostic API for traversing the contents of a dir.
*/

typedef struct FileList FileList;

/* Obtain a list of files in the given <path>, taking care to filter out
 * files whose file names do not end in <suffix>.
*/
extern FileList *	filelist_new(const char *path, const char *suffix);

/* The number of filenames in <fl>. */
extern size_t		filelist_size(const FileList *fl);

/* Get a filename from <fl> by (zero-based) <index>. */
extern const char *	filelist_filename(const FileList *fl, int index);

/* Get the full filename, i.e. prepend the original path to it as well. */
extern const char *	filelist_filename_full(const FileList *fl, int index);

/* Destroy filelist <fl> when your're done with it. */
extern void		filelist_destroy(FileList *fl);
