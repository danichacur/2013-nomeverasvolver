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
		.getattr = grasa_getattr,
		.readdir  = grasa_readdir,
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

    fd = open(path,O_RDWR);

    if (fd == -1){
     printf("Open Failed!\n");
    }

    if (stat("/home/utnso/Escritorio/disk.bin", &sbuf) == -1) {
    	printf("stat Failed!\n");
    }

    mapeo = mmap(0,sbuf.st_size,PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

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


//grasa_getattr

static int grasa_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	GFile Nodo;

	memset(stbuf, 0, sizeof(struct stat));

	Nodo = obtenerNodo(path);

	if (Nodo.state == 2) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (Nodo.state == 1) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = Nodo.file_size;
	} else
		res = -ENOENT;

	return res;
}

// IMPLEMENTACION DE FUNCIONES FUSE


// - grasa_open -

static int grasa_open(const char *path, struct fuse_file_info *fi) {
	GFile Nodo;

	Nodo = obtenerNodo(path);

	if (Nodo.state == 3)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

// - grasa_read -

static int grasa_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	(void) fi;
	GFile Nodo;
	size_t aCopiar=0;
	tObNroBloque NroBloque;

	Nodo = obtenerNodo(path);

	if (Nodo.state == 3)
		return -ENOENT;

	if (offset + size > Nodo.file_size)
		size = Nodo.file_size - offset;

	if (offset < Nodo.file_size) {

			while(offset < Nodo.file_size){

				NroBloque = obtenerNroBloque(Nodo,offset);

				aCopiar = BLOCK_SIZE - NroBloque.offsetDatos;

				memcpy(buf,obtenerDatos(NroBloque.BloqueDatos,NroBloque.offsetDatos),aCopiar);

				buf += aCopiar;
				offset += aCopiar;
			}
			buf -= size;
	} else
		size = 0;

	return size;
}


// - grasa_readdir -

static int grasa_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	(void) offset;
	(void) fi;
	GFile Nodo;

	Nodo = obtenerNodo(path);

	if (Nodo.state == 3)
		return -ENOENT;

	// "." y ".." son entradas validas, la primera es una referencia al directorio donde estamos parados
	// y la segunda indica el directorio padre
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf,Nodo.fname, NULL, 0);

	return 0;
}


//FUNCIONES AUXILIARES

// - obtenerNodo -
//Entrada: Dado el path.
//Salida: Devuelve el nro de inodo.

GFile obtenerNodo(char *path){

	int i;
	char *Filename,*dir1,*dir2;
	GFile NodoBuscado;

	Filename = basename(strdup(path));
	dir1 = strdup(path);
	dir1 = dirname(dir1);
	dir1 = dirname(dir1);
	dir2 = strdup(path);
	dir2 = dirname(dir2);
	dir2 = basename(dir2);

	//Si tengo un solo nivel de directorio, directorio raiz
	if (strcmp(dir1, "/") == 0){
		for (i = 0; i < 1024; i++){
			if (strcmp(GTNodo[i].fname,Filename) == 0){
				NodoBuscado = GTNodo[i];
				printf("NodoBuscado,fname %s,%s.\n",Filename,NodoBuscado.fname);
				return NodoBuscado;
			}
		}
	}
	else{
		//Si tengo dos niveles de directorios
		for (i = 0; i < 1024; i++){
			if ((strcmp(GTNodo[i].fname,Filename) == 0)&&(strcmp(GTNodo[GTNodo[i].parent_dir_block].fname,dir2) == 0)){
				NodoBuscado = GTNodo[i];
				printf("NodoBuscado,fname %s,%s.\n",Filename,NodoBuscado.fname);
				return NodoBuscado;
			}
		}
	}

	//Si no se ecuentra nodo es devuelve 3 en state, validar este retorno.
	NodoBuscado.state = 3;
	return NodoBuscado;

}

// - obtenerNroBloque -
//Entrada: dado el nro de inodo y un offset del archivo a procesar.
//Salida: offset del bloque de datos y bloque de datos.

tObNroBloque obtenerNroBloque(ptrGBloque NroNodo, off_t offsetArchivo){

	off_t entero,indirecto;
	tObNroBloque admNroBloque;

	entero = offsetArchivo / BLOCK_SIZE;
	indirecto = entero / GFILEBYTABLE;

	//Se calcula el resto y se resta 1 para contar desde 0.
	admNroBloque.offsetDatos = offsetArchivo % BLOCK_SIZE;
	if (entero) --admNroBloque.offsetDatos;

    blqindatos = (ptrGBloque*)(mapeo + GTNodo[NroNodo].blk_indirect[indirecto]*BLOCK_SIZE);

    admNroBloque.BloqueDatos = *(blqindatos+entero);

	return admNroBloque;

}

// - obtenerDatos -
//Entrada: dado el nro de bloque de datos y un offset del archivo bloque de datos.
//Salida: datos desde el offset de entrada.

char * obtenerDatos(ptrGBloque NroBloqueDatos, off_t offsetbloque){

	char * datos = (mapeo + NroBloqueDatos*BLOCK_SIZE + offsetbloque);

	return datos;

}

