#include <stdint.h>
#include <fuse.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // aca esta memset
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

/* Librerias de Commons */
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <collections/list.h>


//Constantes

#define GFILEBYTABLE 1024
#define GFILEBYBLOCK 1
#define GFILENAMELENGTH 71
#define GHEADERBLOCKS 1
#define BLKINDIRECT 1000
#define BLOCK_SIZE 4096

//Estructuras

typedef uint32_t ptrGBloque;

typedef struct grasa_header_t { // un bloque
	unsigned char grasa[5];
	uint32_t version;
	uint32_t blk_bitmap;
	uint32_t size_bitmap; // en bloques
	unsigned char padding[4073];
} GHeader;

typedef struct grasa_file_t { // un cuarto de bloque (256 bytes)
	uint8_t state; // 0: borrado, 1: archivo, 2: directorio
	unsigned char fname[GFILENAMELENGTH];
	uint32_t parent_dir_block;
	uint32_t file_size;
	uint64_t c_date;
	uint64_t m_date;
	ptrGBloque blk_indirect[BLKINDIRECT];
} GFile;

typedef struct grasa_adm_t { // Estructura Administracion Grasa
	GHeader admHeader;
	unsigned char admbitmap[1024];
	GFile admTnodo[1024];
	//Bloque datos
} GAdm;

//Variable Global estructura FS Grasa
GAdm *GAdmin;

//Prototipos

/*fuse functions */
static uint32_t grasa_getattr(const char *path, struct stat *statbuf);
static uint32_t grasa_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,struct fuse_file_info *fi);
static uint32_t grasa_open(const char *path, struct fuse_file_info *fi);
static uint32_t grasa_release(const char *path, struct fuse_file_info *fi);
static uint32_t grasa_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static uint32_t grasa_flush(const char *path, struct fuse_file_info *fi);
static uint32_t grasa_mkdir(const char *path, mode_t mode);
static uint32_t grasa_rmdir(const char *path);
static uint32_t grasa_unlink(const char *path);
static uint32_t grasa_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static uint32_t grasa_truncate(const char *path, off_t newsize);
static uint32_t grasa_create(const char *path, mode_t mode, struct fuse_file_info *fi);
static uint32_t grasa_rename(const char *path, const char *newPath);

