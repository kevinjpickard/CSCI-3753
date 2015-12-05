// Copied for use with PA5 for CSCI 3753: Operating Systems
/*
Edited By: Kevin Pickard
Date: 11/30/2015
CSCI 3753: Operating Systems
PA5: Encrypted Mirroring File Systems

Worked with: Michelle Bray
*/

/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>

  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags` fusexmp.c -o fusexmp `pkg-config fuse --libs`

  Note: This implementation is largely stateless and does not maintain
        open file handels between open and release calls (fi->fh).
        Instead, files are opened and closed as necessary inside read(), write(),
        etc calls. As such, the functions that rely on maintaining file handles are
        not implmented (fgetattr(), etc). Those seeking a more efficient and
        more complete implementation may wish to add fi->fh support to minimize
        open() and close() calls and support fh dependent functions.

*/

#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <sys/time.h>
#include <sys/types.h>

#include "aes-crypt.h"

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#include <stdlib.h>

#endif

#define ENCRYPT 	1
#define DECRYPT 	0
#define COPY 		-1	
#define XATRR_ENCRYPTED_FLAG "user.pa5-encfs.encrypted"
#define XATTR_FILE_PASSPHRASE "user.pa5-encfs.passphrase"

// Usage Prototype
#define USAGE "Usage:\n\t./pa5-encfs <Mount Point> <Mirror Directory> <Passkey>\n"

#define ENCFS_DATA ((struct encfs_state *) fuse_get_context()->private_data)

struct encfs_state {
	char *mirror_dir;
	char *passkey;
};

static char * ftemp(const char *path, const char *suffix){
	char * temp_path;
	int len = strlen(path) + strlen(suffix) + 1; 
	if( !(temp_path = malloc(sizeof(char)* len)) ){
		fprintf(stderr, "Error allocating temproary file in %s function.\n", suffix);
		exit(EXIT_FAILURE);
	}
	temp_path[0] = '\0';
	strcat(strcat(temp_path,path),suffix);
	return temp_path;
}

static void xmp_getfullpath(char fpath[PATH_MAX], const char *path)
{
    	strcpy(fpath, ENCFS_DATA->mirror_dir);
    	strncat(fpath, path, PATH_MAX); 
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res=0;
	int action = COPY;
	ssize_t vsize = 0;
	char *tval = NULL;

	time_t    atime;   	/* time of last access */
	time_t    mtime;   	/* time of last modification */
    time_t    tctime;   	/* time of last status change */
    dev_t     t_dev;     	/* ID of device containing file */
    ino_t     t_ino;     	/* inode number */
    mode_t    mode;    	/* protection */
    nlink_t   t_nlink;   	/* number of hard links */
    uid_t     t_uid;     	/* user ID of owner */
    gid_t     t_gid;     	/* group ID of owner */
    dev_t     t_rdev;    	/* device ID (if special file) */

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	res = lstat(fpath, stbuf);
	if (res == -1){
		return -errno;
	}
	
	/* is it a regular file? */
	if (S_ISREG(stbuf->st_mode)){

		/* These file characteristics don't change after decryption so just storing them */
		atime = stbuf->st_atime;
		mtime = stbuf->st_mtime;
		tctime = stbuf->st_ctime;
		t_dev = stbuf->st_dev;
		t_ino = stbuf->st_ino;
		mode = stbuf->st_mode;
		t_nlink = stbuf->st_nlink;
		t_uid = stbuf->st_uid;
		t_gid = stbuf->st_gid;
		t_rdev = stbuf->st_rdev;

		/* Get size of flag value and value itself */
		vsize = getxattr(fpath, XATRR_ENCRYPTED_FLAG, NULL, 0);
		tval = malloc(sizeof(*tval)*(vsize));
		vsize = getxattr(fpath, XATRR_ENCRYPTED_FLAG, tval, vsize);
		
		fprintf(stderr, "Xattr Value: %s\n", tval);

		/* If the specified attribute doesn't exist or it's set to false */
		if (vsize < 0 || memcmp(tval, "false", 5) == 0){
			if(errno == ENODATA){
				fprintf(stderr, "No %s attribute set\n", XATRR_ENCRYPTED_FLAG);
			}
			fprintf(stderr, "File unencrypted, reading...\n");
			action = COPY;
		}
		/* If the attribute exists and is true get size of decrypted file */
		else if (memcmp(tval, "true", 4) == 0){
			fprintf(stderr, "file encrypted, decrypting...\n");
			action = DECRYPT;
		}

		const char *tpath = ftemp(fpath, ".getattr");
		FILE *dfd = fopen(tpath, "wb+");
		FILE *fd = fopen(fpath, "rb");

		fprintf(stderr, "fpath: %s\ntpath: %s\n", fpath, tpath);

		if(!do_crypt(fd, dfd, action, ENCFS_DATA->passkey)){
			fprintf(stderr, "getattr do_crypt failed\n");
    		}

		fclose(fd);
		fclose(dfd);

		/* Get size of decrypted file and store in stat struct */
		res = lstat(tpath, stbuf);
		if (res == -1){
			return -errno;
		}

		/* Put info about file into stat struct*/
		stbuf->st_atime = atime;
		stbuf->st_mtime = mtime;
		stbuf->st_ctime = tctime;
		stbuf->st_dev = t_dev;
		stbuf->st_ino = t_ino;
		stbuf->st_mode = mode;
		stbuf->st_nlink = t_nlink;
		stbuf->st_uid = t_uid;
		stbuf->st_gid = t_gid;
		stbuf->st_rdev = t_rdev;

		free(tval);
		remove(tpath);
	}
	
	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	res = access(fpath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	res = readlink(fpath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	dp = opendir(fpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(fpath, mode);
	else
		res = mknod(fpath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	res = mkdir(fpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	res = unlink(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	res = rmdir(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;

	char ffrom[PATH_MAX], fto[PATH_MAX];
	xmp_getfullpath(ffrom, from);
	xmp_getfullpath(fto, to);

	res = symlink(ffrom, fto);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;

	char ffrom[PATH_MAX], fto[PATH_MAX];
	xmp_getfullpath(ffrom, from);
	xmp_getfullpath(fto, to);

	res = rename(ffrom, fto);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;

	char ffrom[PATH_MAX], fto[PATH_MAX];
	xmp_getfullpath(ffrom, from);
	xmp_getfullpath(fto, to);

	res = link(ffrom, fto);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int res;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	res = chmod(fpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	res = lchown(fpath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	int res;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	res = truncate(fpath, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	res = utimes(fpath, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	res = open(fpath, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	(void)fi;
	int res=0;
	int action;
	ssize_t vsize = 0;
	char *tval = NULL;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	vsize = getxattr(fpath, XATRR_ENCRYPTED_FLAG, NULL, 0);
	tval = malloc(sizeof(*tval)*(vsize));
	vsize = getxattr(fpath, XATRR_ENCRYPTED_FLAG, tval, vsize);

	fprintf(stderr, "Size: %zu, offset: %zu.\n", size, offset);

	/* If the specified attribute doesn't exist or it's set to false */
	if (vsize < 0 || memcmp(tval, "false", 5) == 0){
		if(errno == ENODATA){
			fprintf(stderr, "Read: No %s attribute set\n", XATRR_ENCRYPTED_FLAG);
		}
		action = COPY;
	}
	else if (memcmp(tval, "true", 4) == 0){
		action = DECRYPT;
		fprintf(stderr, "Read: file is encrypted, need to decrypt\n");
	}

	const char *tpath = ftemp(fpath, ".read");
	FILE *dfd = fopen(tpath, "wb+");
	FILE *fd = fopen(fpath, "rb");

	if(!do_crypt(fd, dfd, action, ENCFS_DATA->passkey)){
		fprintf(stderr, "Encryption failed, error code: %d\n", res);
    	}

    fseek(dfd, 0, SEEK_END);
   	size_t tflen = ftell(dfd);
    fseek(dfd, 0, SEEK_SET);

   	res = fread(buf, 1, tflen, dfd);
    	if (res == -1) res = -errno;

	fclose(fd);
	fclose(dfd);
	remove(tpath);
	free(tval);

	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	(void) fi;
	(void) offset;

	int res=0;
	int action = COPY;

	ssize_t vsize = 0;
	char *tval = NULL;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	vsize = getxattr(fpath, XATRR_ENCRYPTED_FLAG, NULL, 0);
	tval = malloc(sizeof(*tval)*(vsize));
	vsize = getxattr(fpath, XATRR_ENCRYPTED_FLAG, tval, vsize);

	if (vsize < 0 || memcmp(tval, "false", 5) == 0){
		if(errno == ENODATA){
			fprintf(stderr, "Encryption flag not set, file cannot be read.\n");
		}
		fprintf(stderr, "File unencrypted, reading...\n");
	}
	/* If the attribute exists and is true get size of decrypted file */
	else if (memcmp(tval, "true", 4) == 0){
		fprintf(stderr, "File encrypted, decrypting...\n");
		action = DECRYPT;
	}


	/* If the file to be written to is encrypted */
	if (action == DECRYPT){
		
		FILE *fd = fopen(fpath, "rb+");
		const char *tpath = ftemp(fpath, ".write");
		FILE *dfd = fopen(tpath, "wb+");

		fseek(fd, 0, SEEK_END);
		fseek(fd, 0, SEEK_SET);

		if(!do_crypt(fd, dfd, DECRYPT, ENCFS_DATA->passkey)){
			fprintf(stderr, "Decryption failed, error code: %d\n", res);
    	}

    	fseek(fd, 0, SEEK_SET);

    	res = fwrite(buf, 1, size, dfd);
    	if (res == -1)
		res = -errno;

		fseek(dfd, 0, SEEK_SET);

		if(!do_crypt(dfd, fd, ENCRYPT, ENCFS_DATA->passkey)){
			fprintf(stderr, "Encryption failed, error code: %d\n", res);
		}

		fclose(fd);
		fclose(dfd);
		remove(tpath);
	}
	/* If the file to be written to is unencrypted */
	else if (action == COPY){
		int fd1;
		fprintf(stderr, "File unencrypted, reading...\n");

		fd1 = open(fpath, O_WRONLY);
		if (fd1 == -1)
			return -errno;

		res = pwrite(fd1, buf, size, offset);
		if (res == -1)
			res = -errno;

		close(fd1);
   	}
   	
	free(tval);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;

	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	res = statvfs(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi) {
	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);
	
    (void) fi;
    (void) mode;

	FILE *fd = fopen(fpath, "wb+");

	fprintf(stderr, "CREATE: fpath: %s\n", fpath);

	if(!do_crypt(fd, fd, ENCRYPT, ENCFS_DATA->passkey)){
		fprintf(stderr, "Create: do_crypt failed\n");
    }

	fprintf(stderr, "Create: encryption done correctly\n");

	fclose(fd);

	if(setxattr(fpath, XATRR_ENCRYPTED_FLAG, "true", 4, 0)){
    	fprintf(stderr, "error setting xattr of file %s\n", fpath);
    	return -errno;
   	}
   	fprintf(stderr, "Create: file xatrr correctly set %s\n", fpath);
   
    return 0;
}


static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	int res = lsetxattr(fpath, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	int res = lgetxattr(fpath, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	int res = llistxattr(fpath, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	char fpath[PATH_MAX];
	xmp_getfullpath(fpath, path);

	int res = lremovexattr(fpath, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.create     = xmp_create,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	umask(0);

	if(argc<4){
		fprintf(stderr, "Incorrect usage, please try again.\n\n\t%s\n\n", USAGE);
		exit(EXIT_FAILURE);
	}

	struct encfs_state *encfs_data;

	encfs_data = malloc(sizeof(struct encfs_state));
    if(encfs_data == NULL) {
		perror("Error allocating heap.");
		exit(EXIT_FAILURE);
    }

	// Parsing input args
	encfs_data->mirror_dir = realpath(argv[2], NULL);
	encfs_data->passkey = argv[3];
	argv[2] = NULL;
	argv[3] = NULL;
	argc = argc - 2;
	char *mount_dir = argv[1];

	fprintf(stdout, "Mounting %s to %s\n", encfs_data->mirror_dir, mount_dir);

	int res;

	// Initializing filesystem
	if( (res=fuse_main(argc, argv, &xmp_oper, encfs_data)) ) {
		fprintf(stderr, "Internal FUSE error, please try again.\n");
		exit(EXIT_FAILURE);
	}

	// SUCCESS!
	else printf("Filesystem successfully initialized.\n");
	return res;
}