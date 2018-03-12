#define _GNU_SOURCE

#define __NR_memfd_create 385
#define SYS_memfd_create __NR_memfd_create

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <glob.h>
#include <stdarg.h>
#include <sys/sendfile.h>

char *patch_base_path = "/usr/share/prepatch/";

typedef int (*orig_open_f_type)(const char *pathname, int flags, ...);

void patchFile(const char *filePath, const char *patchPath)
{
	if (!fork())
	{
		execlp("patch", "patch", "-s", filePath, patchPath, NULL);
	}
	wait(NULL);
}

char* findReplacement(char* origPath) {
	int bufLen = strlen(patch_base_path) + 1 + strlen(origPath) + 1;
	char* globQuery = malloc(bufLen);
	sprintf(globQuery, "%s*%s", patch_base_path, origPath);
	glob_t globlist;
	if (glob(globQuery, GLOB_PERIOD, NULL, &globlist) == GLOB_NOSPACE || glob(globQuery, GLOB_PERIOD, NULL, &globlist) == GLOB_NOMATCH) {
		free(globQuery);
		globfree(&globlist);	
		return origPath;
	}
	if (glob(globQuery, GLOB_PERIOD, NULL, &globlist) == GLOB_ABORTED) {
		free(globQuery);
		globfree(&globlist);	
		return origPath;
	}
	if(globlist.gl_pathv[0]) {
		free(globQuery);
		char* repStr = malloc(strlen(globlist.gl_pathv[0])+1);
		memcpy(repStr, globlist.gl_pathv[0], strlen(globlist.gl_pathv[0])+1);
		globfree(&globlist);
		return repStr;
	} else {
		free(globQuery);
		globfree(&globlist);	
		return origPath;
	}
}


int handle_open(orig_open_f_type orig_open, const char *relpath, int flags)
{
	if ( (flags & O_APPEND) || (flags & O_WRONLY) || getuid() != 100000  || (strncmp("/sys", relpath, strlen("/sys")) == 0) || (strncmp("/dev", relpath, strlen("/dev")) == 0) || (strncmp("/run", relpath, strlen("/run")) == 0)) {
		// Don't run if trying to write or not nemo
		return (orig_open(relpath, flags));
	}

	
	char *pathname = malloc(PATH_MAX);
	realpath(relpath, pathname);

	// Replace (or add) the file if needed
	char* newPath = findReplacement(pathname);
	int new = 0;
	if(strcmp(newPath,pathname)) {
		new = 1;
		pathname = newPath;
		relpath = newPath;
	} 
	



	// Apply patches (if needed)
	int fd;
	int bufLen = strlen(patch_base_path) + 1 + strlen(pathname) + strlen(".patch") + 1;
	char *globQuery = malloc(bufLen);
	sprintf(globQuery, "%s*%s.patch", patch_base_path, pathname);
	glob_t globlist;
	if ((glob(globQuery, GLOB_PERIOD, NULL, &globlist) == GLOB_NOSPACE || glob(globQuery, GLOB_PERIOD, NULL, &globlist) == GLOB_NOMATCH) || (glob(globQuery, GLOB_PERIOD, NULL, &globlist) == GLOB_ABORTED)) {
		fd = orig_open(relpath, flags);
		if(new)
			free(newPath);
		return fd;
	}
	if(globlist.gl_pathv[0]) {
		char *outPath = tmpnam(NULL);
		int fdOr = orig_open(relpath, flags);
		struct stat stat_buf; 
		fstat(fdOr, &stat_buf);
		int fdOut = orig_open(outPath, O_CREAT | O_WRONLY, 0777);
		sendfile(fdOut, fdOr, NULL, stat_buf.st_size);
		close(fdOr);
		close(fdOut);

		int i;
		for(i = 0; globlist.gl_pathv[i]; i++)
		{
			patchFile(outPath, globlist.gl_pathv[i]);			
		}

		fd = orig_open(outPath, flags);		
		unlink(outPath);
	} else {
		fd = orig_open(relpath, flags);
	}
	free (globQuery);
	globfree(&globlist);
	if(new)
		free(newPath);	
	return fd; 

}


int open64(const char *pathname, int flags, ...)
{

    orig_open_f_type orig_func;
    orig_func = (orig_open_f_type)dlsym(RTLD_NEXT, "open64");

    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        int mode = va_arg(args, int);
        va_end(args);
        return orig_func(pathname, flags, mode);
    } else {
        return handle_open(orig_func, pathname, flags);
    }
}


int open(const char *pathname, int flags, ...)
{

    orig_open_f_type orig_func;
    orig_func = (orig_open_f_type)dlsym(RTLD_NEXT, "open");

    // If O_CREAT is used to create a file, the file access mode must be given.
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        int mode = va_arg(args, int);
        va_end(args);
        return orig_func(pathname, flags, mode);
    } else {
        return handle_open(orig_func, pathname, flags);
    }
}


/*
int open(const char *pathname, int flags, ...)
{
	orig_open_f_type orig_open;
	orig_open = (orig_open_f_type)dlsym(RTLD_NEXT, "open");
	if ((flags & O_CREAT) || (flags & O_TMPFILE))
	{
		va_list arg;
		va_start(arg, flags);
		mode_t mode = va_arg(arg, int);
		va_end(arg);
		return (orig_open(pathname, flags, mode));
	}
	return handle_open(orig_open, pathname, flags);
}
*/

/*
int __libc_start_main(int (*main) (int, char **, char **),
		      int argc, char **ubp_av, void (*init) (void),
		      void (*fini) (void), void (*rtld_fini) (void),
		      void (*stack_end))
{
    int (*orig_start_main) (int (*main) (int, char **, char **),
		      int argc, char **ubp_av, void (*init) (void),
		      void (*fini) (void), void (*rtld_fini) (void),
		      void (*stack_end)); 
    
    orig_start_main = dlsym(RTLD_NEXT, "__libc_start_main");
    puts("Hello world!");
    return (*orig_start_main) (main, argc, ubp_av, init, fini, rtld_fini, stack_end);
}*/
