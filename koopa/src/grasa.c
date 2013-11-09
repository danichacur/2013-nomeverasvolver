/*
 ============================================================================
 Name        : grasa.c
 Author      : PA
 Version     :
 Copyright   : Your copyright notice
 Description : GRASA Filesystem
 ============================================================================
 */

#include "grasa.h"

/*
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
*/


int main(void) {

	int fd;
	struct stat sbuf;

	//prueba
    char* path = "/home/utnso/Escritorio/disk.bin";

    fd = open(path,O_RDONLY);

    if (fd == -1){
     printf("Open Failed!\n");
    }

    if (stat("/home/utnso/Escritorio/disk.bin", &sbuf) == -1) {
    	printf("stat Failed!\n");
    }

    GAdmin = (GAdm*) mmap(0,sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);

    if (GAdmin == MAP_FAILED){
     printf("map Failed!\n");
    }


    puts("Header");
    printf("Contenido: %s\n",GAdmin->admHeader.grasa);
    printf("Version: %u\n",GAdmin->admHeader.version);
    printf("Bloque inicio: %u\n",GAdmin->admHeader.blk_bitmap);
    printf("Tamaño: %u\n",GAdmin->admHeader.size_bitmap);

    GBitmap = bitarray_create(GAdmin->admbitmap, sbuf.st_size/BLOCK_SIZE/8);
    GTNodo = GAdmin->admTnodo;

    printf("Bitmap-pos: %u\n",bitarray_get_max_bit(GBitmap));
    printf("nombre: %s\n",GTNodo[6].fname);
    printf("dirblock: %u\n",GTNodo[6].parent_dir_block);
    printf("estado: %u\n",GTNodo[6].state);
    printf("tamanio: %u\n",GTNodo[6].file_size);
    printf("blq 5: %s\n",GTNodo[5].fname);

    return EXIT_SUCCESS;
    //return fuse_main(argc, argv, &fuse_oper,NULL)

}

/*
 * @DESC
 *  Esta funciÃ³n va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para obtener la metadata de un archivo/directorio. Esto puede ser tamaÃ±o, tipo,
 * permisos, dueÃ±o, etc ...
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		stbuf - Esta esta estructura es la que debemos completar
 *
 * 	@RETURN
 * 		O archivo/directorio fue encontrado. -ENOENT archivo/directorio no encontrado
 */


/*
static int grasa_getattr(const char *path, struct stat *stbuf) {
	int res = 0;

	//memset(stbuf, 0, sizeof(struct stat));

	//Si path es igual a "/" nos estan pidiendo los atributos del punto de montaje

	//if (strcmp(path, "/") == 0) {
	//	stbuf->st_mode = S_IFDIR | 0755;
	//	stbuf->st_nlink = 2;
	//}

	//a construir ... Revisa en la estructura que es.
	//entryDir = dameDirEntry(path);


	//a construir.... Verifico si es nulo
	//if (entryDir == NULL){
	//	log_error( pInfo->log, "thread-1", "%s : [%s]","No se encontro los atributos para el path",path);
	//	return -ENOENT;
	//}

	//Si es un directorio, estado 2, sino es un archivo.
	//construir dameNumCLuster, obtenerDirTables
	// construir usando commons liberarListaDirEntry
	//if( entryDir->state = 2){

		//stbuf->st_mode = S_IFDIR | 0755;
		//stbuf->st_size = pInfo->pboot_sector->byteXsector * pInfo->pboot_sector->sectorXcluster;
		//lista = obtenerDirTables(dameNumCLuster(entryDir));
		//cantHardLinks = lenghtList(lista);
		//liberarListaDirEntry(lista);
		//stbuf->st_nlink = cantHardLinks +2;

	//}else {

		//stbuf->st_mode = S_IFREG | 0755;
		//stbuf->st_nlink = 1;
		//stbuf->st_size = entryDir->size;

	//}

	return res;
}

int grasa_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;

    return retstat;
}
*/

//Obtengo la información del tamaño del archivo y los punteros a los bloques de datos

GFile* obtenerNodo(char *path){

	int i;
	char *Filename;
	GFile *NodoBuscado;

	Filename = basename(path);

	 for (i = 0; i < 1024; i++){
	      if (strcmp(GTNodo[i]->fname,Filename) == 0){
	    	 NodoBuscado = GTNodo;
	    	 printf("NodoBuscado,fname %s.\n",Filename,NodoBuscado->fname);
	    	 return NodoBuscado;
	      }
	   }


	return -1;

}

//Para que te devuelva el número de bloque lineal donde está bloque "i" de datos del archivo.

uint32_t bloque obtenerNroBloque(nodo, i);




