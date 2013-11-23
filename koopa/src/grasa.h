#include <stdint.h>
#include <fuse.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // aca esta memset
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

/* Librerias de Commons */
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/bitarray.h>
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
	char fname[GFILENAMELENGTH];
	uint32_t parent_dir_block;
	uint32_t file_size;
	uint64_t c_date;
	uint64_t m_date;
	ptrGBloque blk_indirect[BLKINDIRECT];
} GFile;

// Estructura Administracion Grasa
typedef struct grasa_adm_t {
	GHeader admHeader;
	char admbitmap[BLOCK_SIZE];
	GFile admTnodo[GFILEBYTABLE];
} GAdm;

//Estructura funcion obtenerNodo
typedef struct grasa_obtenerNodo_t {
	GFile NodoBuscado;
	ptrGBloque NroNodo;
} tNodoBuscado;

//Estructura funcion obtenerNroBloque
typedef struct grasa_obtenerNroBloque_t {
	ptrGBloque BloqueDatos;
	off_t offsetDatos;
} tObNroBloque;


//Variable Global estructura FS Grasa
GAdm *GAdmin;
GFile *GTNodo;
t_bitarray *GBitmap;
ptrGBloque *blqindatos;
char * mapeo; //Disco mapeado Global

//Prototipos
//Funciones auxiliares
tNodoBuscado obtenerNodo(const char *path,uint32_t j);
tObNroBloque obtenerNroBloque(ptrGBloque NroNodo, off_t offsetArchivo);
char * obtenerDatos(ptrGBloque NroBloqueDatos, off_t offsetbloque);
/*fuse functions */

static uint32_t grasa_getattr(const char *path, struct stat *statbuf); //Obtener atributos
static uint32_t grasa_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,struct fuse_file_info *fi); //Leer contenido de directorio
static uint32_t grasa_open(const char *path, struct fuse_file_info *fi); //Verificacion de apertura de archivo
static uint32_t grasa_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi); //Lectura del archivo desde offset dado
static uint32_t grasa_mkdir(const char *path, mode_t mode); //Crear directorio
static uint32_t grasa_rmdir(const char *path); //Borrar Directorio vacio
static uint32_t grasa_unlink(const char *path); //Unlink(Borrar) Archivo
static uint32_t grasa_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static uint32_t grasa_truncate(const char *path, off_t newsize);//Truncar archivo
static uint32_t grasa_create(const char *path, mode_t mode, struct fuse_file_info *fi);//crear archivo vacio

