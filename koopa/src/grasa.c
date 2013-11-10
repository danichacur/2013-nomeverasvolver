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


static struct fuse_operations fuse_oper = {
		.open = grasa_open,
		//.getattr = grasa_getattr,
		.readdir  = grasa_readdir,
		//.release = grasa_release,
		//.flush = grasa_flush,
		.read = grasa_read,
		.mkdir = grasa_mkdir,
		.rmdir = grasa_rmdir,
		.unlink = grasa_unlink,
		.write = grasa_write,
		.truncate = grasa_truncate,
		.create = grasa_create
		//.rename = grasa_rename
};


/*
 * Esta es una estructura auxiliar utilizada para almacenar parametros
 * que nosotros le pasemos por linea de comando a la funcion principal
 * de FUSE
 */
struct t_runtime_options {
	char* welcome_msg;
} runtime_options;


int main(int argc, char *argv[]) {

	int fd;
	struct stat sbuf;

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	// Limpio la estructura que va a contener los parametros
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	// Esta funcion de FUSE lee los parametros recibidos y los intepreta
	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1){
		/** error parsing options */
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}

	//prueba
    char* path = "/home/utnso/Escritorio/disk.bin";

    fd = open(path,O_RDONLY);

    if (fd == -1){
     printf("Open Failed!\n");
    }

    if (stat("/home/utnso/Escritorio/disk.bin", &sbuf) == -1) {
    	printf("stat Failed!\n");
    }

    mapeo = mmap(0,sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);

    if (mapeo == MAP_FAILED){
     printf("map Failed!\n");
    }

    GAdmin = (GAdm*) mapeo;
    GTNodo = GAdmin->admTnodo;
    GBitmap = bitarray_create(GAdmin->admbitmap, sbuf.st_size/BLOCK_SIZE/8);

	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comuniscarse con el kernel, delegar
    // en varios threads
	return fuse_main(args.argc, args.argv, &fuse_oper, NULL);


    return EXIT_SUCCESS;

}

/** keys for FUSE_OPT_ options */
enum {
	KEY_VERSION,
	KEY_HELP,
};


/*
 * Esta estructura es utilizada para decirle a la biblioteca de FUSE que
 * parametro puede recibir y donde tiene que guardar el valor de estos
 */
static struct fuse_opt fuse_options[] = {
		// Este es un parametro definido por nosotros
		//CUSTOM_FUSE_OPT_KEY("--welcome-msg %s", welcome_msg, 0),

		// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};

/* - grasa_read -
 * @DESC
 *  Esta funciÃ³n va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para obtener el contenido de un archivo
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		buf - Este es el buffer donde se va a guardar el contenido solicitado
 * 		size - Nos indica cuanto tenemos que leer
 * 		offset - A partir de que posicion del archivo tenemos que leer
 *
 * 	@RETURN
 * 		Si se usa el parametro direct_io los valores de retorno son 0 si  elarchivo fue encontrado
 * 		o -ENOENT si ocurrio un error. Si el parametro direct_io no esta presente se retorna
 * 		la cantidad de bytes leidos o -ENOENT si ocurrio un error. ( Este comportamiento es igual
 * 		para la funcion write )
 */

int grasa_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;

    return retstat;
}

/* - grasa_readdir
 * @DESC
 *  Esta funciÃ³n va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para obtener la lista de archivos o directorios que se encuentra dentro de un directorio
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		buf - Este es un buffer donde se colocaran los nombres de los archivos y directorios
 * 		      que esten dentro del directorio indicado por el path
 * 		filler - Este es un puntero a una funciÃ³n, la cual sabe como guardar una cadena dentro
 * 		         del campo buf
 *
 * 	@RETURN
 * 		O directorio fue encontrado. -ENOENT directorio no encontrado
 */
static int grasa_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	// "." y ".." son entradas validas, la primera es una referencia al directorio donde estamos parados
	// y la segunda indica el directorio padre
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, DEFAULT_FILE_NAME, NULL, 0);

	return 0;
}


//Obtengo el nodo

GFile* obtenerNodo(char *path){

	int i;
	char *Filename,*dir1,*dir2;
	GFile *NodoBuscado = NULL;

	Filename = basename(strdup(path));
	dir1 = strdup(path);
	dir1 = dirname(dir1);
	dir1 = dirname(dir1);
	dir2 = strdup(path);
	dir2 = dirname(dir2);
	dir2 = basename(dir2);

	//Si tengo un solo nivel de directorio, directorio raiz
	if (dir1 == "/"){
		for (i = 0; i < 1024; i++){
			if (strcmp(GTNodo[i]->fname,Filename) == 0){
				NodoBuscado = GTNodo[i];
				printf("NodoBuscado,fname %s.\n",Filename,NodoBuscado->fname);
				return NodoBuscado;
			}
		}
	}
	else{
		//Si tengo dos niveles de directorios
		for (i = 0; i < 1024; i++){
			if ((strcmp(GTNodo[i]->fname,Filename) == 0)&&(strcmp(GTNodo[GTNodo[i]->parent_dir_block]->fname,dir2) == 0)){
				NodoBuscado = GTNodo[i];
				printf("NodoBuscado,fname %s.\n",Filename,NodoBuscado->fname);
				return NodoBuscado;
			}
		}
	}

	return -1;

}

//Para que te devuelva el número de bloque lineal donde está bloque "i" de datos del archivo.

ptrGBloque obtenerNroBloque(nodo, i){

	return 0;

}


