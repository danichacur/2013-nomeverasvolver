/*
 ============================================================================
 Name        : grasa.c
 Author      : PA
 Version     :
 Copyright   : Your copyright notice
 Description : GRASA Filesystem
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <grasa.h>

static struct fuse_operations fuse_oper = {
		.open = grasa_open,
		.getattr = grasa_getattr,
		.readdir  = grasa_readdir,
		.release = grasa_release,
		.flush = grasa_flush,
		.read = grasa_read,
		.mkdir = grasa_mkdir,
		.rmdir = grasa_rmdir,
		.unlink = grasa_unlink,
		.write = grasa_write,
		.truncate = grasa_truncate,
		.create = grasa_create,
		.rename = grasa_rename
};

int main(void) {

	return EXIT_SUCCESS;
}
