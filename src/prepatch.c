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
char curPath [PATH_MAX];


char *tmpDir = "/tmp";
char *tmpPrefix = "prepatch";

void patchFile(const char *filePath, const char *patchPath)
{
	if (!fork())
	{
		execlp("patch", "patch", "-s", filePath, patchPath, NULL);
	}
	wait(NULL);
}

int findReplacement() {
	int bufLen = strlen(patch_base_path) + 1 + strlen(curPath) + 1;
	char* globQuery = malloc(bufLen);
	sprintf(globQuery, "%s*%s", patch_base_path, curPath);
	glob_t globlist;
	if (glob(globQuery, GLOB_PERIOD, NULL, &globlist) == GLOB_NOSPACE || glob(globQuery, GLOB_PERIOD, NULL, &globlist) == GLOB_NOMATCH) {
		free(globQuery);
		globfree(&globlist);
		return 0;	
	}
	if (glob(globQuery, GLOB_PERIOD, NULL, &globlist) == GLOB_ABORTED) {
		free(globQuery);
		globfree(&globlist);	
		return 0;
	}
	if(globlist.gl_pathv[0]) {
		free(globQuery);
		memcpy(curPath, globlist.gl_pathv[0], strlen(globlist.gl_pathv[0])+1);
		globfree(&globlist);
		return 1;
	} else {
		free(globQuery);
		globfree(&globlist);	
		return 0;
	}
}


int handle_open(const char *relpath, int flags)
{
	if ( (flags & O_APPEND) || (flags & O_WRONLY) || getuid() != 100000  || (strncmp("/sys", relpath, strlen("/sys")) == 0) || (strncmp("/dev", relpath, strlen("/dev")) == 0) || (strncmp("/run", relpath, strlen("/run")) == 0)) {
		// Don't run if trying to write or not nemo
		return (syscall(5, relpath, flags));
	}

		
	realpath(relpath, curPath);

	// Apply patches (if needed)
	int fd;
	int bufLen = strlen(patch_base_path) + 1 + strlen(curPath) + strlen(".patch") + 1;
	char *globQuery = malloc(bufLen);
	sprintf(globQuery, "%s*%s.patch", patch_base_path, curPath);

	// Replace (or add) the file if needed
	// We're doing this after building the glob query so the path stays the same.
	int new = findReplacement();

	glob_t globlist;
	if ((glob(globQuery, GLOB_PERIOD, NULL, &globlist) == GLOB_NOSPACE || glob(globQuery, GLOB_PERIOD, NULL, &globlist) == GLOB_NOMATCH) || (glob(globQuery, GLOB_PERIOD, NULL, &globlist) == GLOB_ABORTED)) {
		if(new)
			fd = syscall(5, curPath, flags);
		else
			fd = syscall(5, relpath, flags);
		return fd;
	}
	if(globlist.gl_pathv[0]) {
		char *outPath = tempnam(tmpDir, tmpPrefix);
		int fdOr = syscall(5, curPath, flags);
		struct stat stat_buf; 
		fstat(fdOr, &stat_buf);
		int fdOut = syscall(5, outPath, O_CREAT | O_WRONLY, 0777);
		sendfile(fdOut, fdOr, NULL, stat_buf.st_size);
		close(fdOr);
		close(fdOut);

		int i;
		for(i = 0; globlist.gl_pathv[i]; i++)
		{
			patchFile(outPath, globlist.gl_pathv[i]);			
		}

		fd = syscall(5, outPath, flags);		
		unlink(outPath);
		free(outPath);

	} else {
		fd = syscall(5, curPath, flags);
	}
	free (globQuery);
	globfree(&globlist);
	return fd; 

}


int open64(const char *pathname, int flags, ...)
{

    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        int mode = va_arg(args, int);
        va_end(args);
        return syscall(5, pathname, flags, mode);
    } else {
        return handle_open(pathname, flags);
    }
}


int open(const char *pathname, int flags, ...)
{

    // If O_CREAT is used to create a file, the file access mode must be given.
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        int mode = va_arg(args, int);
        va_end(args);
        return syscall(5, pathname, flags, mode);
    } else {
        return handle_open(pathname, flags);
    }
}


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
