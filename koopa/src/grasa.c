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

// IMPLEMENTACION DE FUNCIONES FUSE

//grasa_getattr

static uint32_t grasa_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	tNodoBuscado Nodo;

	memset(stbuf, 0, sizeof(struct stat));

	Nodo = obtenerNodo(path);

	if (Nodo.NodoBuscado.state == 2) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (Nodo.NodoBuscado.state == 1) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = Nodo.NodoBuscado.file_size;
	} else
		res = -ENOENT;

	return res;
}

// - grasa_open -

static uint32_t grasa_open(const char *path, struct fuse_file_info *fi) {
	tNodoBuscado Nodo;

	Nodo = obtenerNodo(path);

	if (Nodo.NodoBuscado.state == 3)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

// - grasa_read -

static uint32_t grasa_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	(void) fi;
	tNodoBuscado Nodo;
	size_t aCopiar=0;
	tObNroBloque NroBloque;

	Nodo = obtenerNodo(path);

	if (Nodo.NodoBuscado.state == 3)
		return -ENOENT;

	if (offset + size > Nodo.NodoBuscado.file_size)
		size = Nodo.NodoBuscado.file_size - offset;

	if (offset <= Nodo.NodoBuscado.file_size) {

			while(offset < Nodo.NodoBuscado.file_size){

				NroBloque = obtenerNroBloque(Nodo.NroNodo,offset);

				aCopiar = BLOCK_SIZE - NroBloque.offsetDatos;

				memcpy(buf,obtenerDatos(NroBloque.BloqueDatos,NroBloque.offsetDatos),aCopiar);

				buf += aCopiar;
				offset += aCopiar;
			}
			//TODO ver si usar size o filesize
			buf -= size;
	} else
		size = 0;

	return size;
}

// - grasa_readdir -

static uint32_t grasa_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	(void) offset;
	(void) fi;
	tNodoBuscado Nodo;

	printf("grasa_readdir");

	Nodo = obtenerNodo(path);

	if (Nodo.NodoBuscado.state == 3)
		return -ENOENT;

	// "." y ".." son entradas validas, la primera es una referencia al directorio donde estamos parados
	// y la segunda indica el directorio padre
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf,Nodo.NodoBuscado.fname, NULL, 0);

	return 0;
}

// - grasa_mkdir -

static uint32_t grasa_mkdir(const char *path, mode_t mode){
	tNodoBuscado Nodo;
	off_t i,j=2;

	printf("grasa_mkdir");

	if(strlen(path) > 71)
		return -ENAMETOOLONG;

	for (i=2; i < 1025; i++){
		j=i;
		i=1025;
		if (bitarray_test_bit(GBitmap,j))
			i=j;
	}

	if(j < 1025){

		GTNodo[j].parent_dir_block = 0;

		if(dirname(strdup(path)) != "/" || dirname(strdup(path)) != "." ){
			Nodo = obtenerNodo(path);
			GTNodo[j].parent_dir_block = Nodo.NodoBuscado.parent_dir_block;
		}

		strcpy(GTNodo[j].fname,basename(path));
		GTNodo[j].file_size = 0;
		GTNodo[j].state = 2;
		GTNodo[j].c_date = (uint64_t)time(NULL);
		GTNodo[j].m_date = (uint64_t)time(NULL);

		bitarray_set_bit(GBitmap,j);

		return 0;
	}


	return -ENOSPC;
}

// - grasa_rmdir -

static uint32_t grasa_rmdir(const char *path){
	tNodoBuscado Nodo;
	int i;

	printf("grasa_rmdir");

	Nodo = obtenerNodo(path);

	if (Nodo.NodoBuscado.state == 3)
		return -ENOENT;

	if (Nodo.NodoBuscado.state < 2)
		return -ENOTDIR;

	for(i=0;i < 1024;i++){
		if(GTNodo[i].parent_dir_block == Nodo.NroNodo)
			return -ENOTEMPTY;
	}

		GTNodo[Nodo.NroNodo].parent_dir_block = 0;
		GTNodo[Nodo.NroNodo].file_size = 0;
		GTNodo[Nodo.NroNodo].state = 0;
		GTNodo[Nodo.NroNodo].c_date = 0;
		GTNodo[Nodo.NroNodo].m_date = 0;

		bitarray_clean_bit(GBitmap,Nodo.NroNodo+2);

	return 0;
}

// - grasa_unlink -

static uint32_t grasa_unlink(const char *path){
	tNodoBuscado Nodo;
	ptrGBloque i=0,j=0;

	printf("grasa_unlink");

	Nodo = obtenerNodo(path);

	if (Nodo.NodoBuscado.state == 3)
		return -ENOENT;

	if (Nodo.NodoBuscado.state == 2 || Nodo.NodoBuscado.state == 0)
		return -EFAULT;

		GTNodo[Nodo.NroNodo].parent_dir_block = 0;
		GTNodo[Nodo.NroNodo].file_size = 0;
		GTNodo[Nodo.NroNodo].state = 0;
		GTNodo[Nodo.NroNodo].c_date = 0;
		GTNodo[Nodo.NroNodo].m_date = 0;

		//Liberar inodo, se coloca +2 ya que el NroNodo es relativo a la tabla de Nodos
		bitarray_clean_bit(GBitmap,Nodo.NroNodo+2);

		j = GTNodo[Nodo.NroNodo].blk_indirect[i];
		while (bitarray_test_bit(GBitmap,j) && i > 1024){
			bitarray_clean_bit(GBitmap,j);
			i++;
			j = GTNodo[Nodo.NroNodo].blk_indirect[i];
		}

	return 0;
}

// - grasa_truncate -

static uint32_t grasa_truncate(const char *path, off_t newsize){
	tNodoBuscado Nodo;

	printf("grasa_truncate");

	Nodo = obtenerNodo(path);

	if (Nodo.NodoBuscado.state == 3)
		return -ENOENT;

	if (Nodo.NodoBuscado.state == 2 || Nodo.NodoBuscado.state == 0)
		return -EFAULT;

		GTNodo[Nodo.NroNodo].file_size = newsize;
		GTNodo[Nodo.NroNodo].m_date =  (uint64_t)time(NULL);

	return 0;
}

static uint32_t grasa_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	(void) fi;
	tNodoBuscado Nodo;
	off_t i,j=2;

	printf("grasa_create");

	if(strlen(path) > 71)
		return -ENAMETOOLONG;

	for (i=2; i < 1025; i++){
		j=i;
		i=1025;
		if (bitarray_test_bit(GBitmap,j))
			i=j;
	}

	if(j < 1025){

		GTNodo[j].parent_dir_block = 0;

		if (strcmp(path, "/") == 0  || strcmp(path, ".") == 0){
			Nodo = obtenerNodo(path);
			GTNodo[j].parent_dir_block = Nodo.NodoBuscado.parent_dir_block;
		}

		strcpy(GTNodo[j].fname,basename(path));
		GTNodo[j].file_size = 0;
		GTNodo[j].state = 1;
		GTNodo[j].c_date = (uint64_t)time(NULL);
		GTNodo[j].m_date = (uint64_t)time(NULL);

		bitarray_set_bit(GBitmap,j);

		return 0;
	}

	return -ENOENT;
}

// - grasa_write -

static uint32_t grasa_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	(void) fi;
	(void) offset;
	tNodoBuscado Nodo;
	size_t aCopiar=0;
	tObNroBloque NroBloque;

	printf("grasa_write");

	Nodo = obtenerNodo(path);

	if (Nodo.NodoBuscado.state == 3)
		return -ENOENT;

	if (offset + size > BLOCK_SIZE*BLKINDIRECT)
		return 0;

	while(offset <= offset + size){

		NroBloque = obtenerNroBloque(Nodo.NroNodo,offset);

		if (bitarray_test_bit(GBitmap,NroBloque.BloqueDatos))
				bitarray_set_bit(GBitmap,NroBloque.BloqueDatos);

		aCopiar = BLOCK_SIZE - NroBloque.offsetDatos;

		memcpy(obtenerDatos(NroBloque.BloqueDatos,NroBloque.offsetDatos),buf,aCopiar);

		buf += aCopiar;
		offset += aCopiar;
	}
	//TODO ver si usar size o filesize
	GTNodo[Nodo.NroNodo].file_size = offset + size;
	GTNodo[Nodo.NroNodo].m_date = (uint64_t)time(NULL);
	buf -= size;

	return size;
}

//FUNCIONES AUXILIARES

// - obtenerNodo -
//Entrada: Dado el path.
//Salida: Devuelve el nro de inodo.

tNodoBuscado obtenerNodo( const char *path){
	int i;
	char *Filename,*dir1,*dir2;
	tNodoBuscado NodoBuscado;

	printf("obtenerNodo");

	Filename = basename(strdup(path));
	dir1 = strdup(path);
	dir1 = dirname(dir1);
	dir1 = dirname(dir1);
	dir2 = strdup(path);
	dir2 = dirname(dir2);
	dir2 = basename(dir2);

	//Si tengo un solo nivel de directorio, directorio raiz
	if (strcmp(dir1, "/") == 0  || strcmp(dir1, ".") == 0){
		for (i = 0; i < 1024; i++){
			if ((strcmp(GTNodo[i].fname,Filename) == 0) && (GTNodo[i].state != 0)){
				NodoBuscado.NodoBuscado = GTNodo[i];
				NodoBuscado.NroNodo = i;
				printf("NodoBuscado,fname %s,%s.\n",Filename,NodoBuscado.NodoBuscado.fname);
				return NodoBuscado;
			}
		}
	}
	else{
		//Si tengo dos niveles de directorios
		for (i = 0; i < 1024; i++){
			if ((strcmp(GTNodo[i].fname,Filename) == 0)&&(strcmp(GTNodo[GTNodo[i].parent_dir_block].fname,dir2) == 0) && (GTNodo[i].state != 0)){
				NodoBuscado.NodoBuscado = GTNodo[i];
				NodoBuscado.NroNodo = i;
				printf("NodoBuscado,fname %s,%s.\n",Filename,NodoBuscado.NodoBuscado.fname);
				return NodoBuscado;
			}
		}
	}

	//Si no se ecuentra nodo es devuelve 3 en state, validar este retorno.
	NodoBuscado.NodoBuscado.state = 3;
	return NodoBuscado;

}

// - obtenerNroBloque -
//Entrada: dado el nro de inodo y un offset del archivo a procesar.
//Salida: offset del bloque de datos y bloque de datos.

tObNroBloque obtenerNroBloque(ptrGBloque NroNodo, off_t offsetArchivo){
	off_t entero,indirecto;
	tObNroBloque admNroBloque;

	printf("obtenerNroBloque");

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

	printf("obtenerDatos");

	char * datos = (mapeo + NroBloqueDatos*BLOCK_SIZE + offsetbloque);

	return datos;

}

static struct fuse_operations grasa_oper = {
		.open = grasa_open,
		.getattr = grasa_getattr,
		.readdir  = grasa_readdir,
		.read = grasa_read,
		.mkdir = grasa_mkdir,
		.rmdir = grasa_rmdir,
		.unlink = grasa_unlink,
		.write = grasa_write,
		.truncate = grasa_truncate,
		.create = grasa_create,
};

//////////////////////////////////////////////MAIN////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
	int fd;
	struct stat sbuf;
	char *pathm="/disk.bin";
	char pathf[71];

	memcpy (pathf,argv[1],strlen(argv[1]));
	pathf[strlen(argv[1])] = '\0';
	strcat(pathf,pathm);

    //path = "/home/utnso/Escritorio/tmp/disk.bin";

    printf("File: %s\n",pathf);

    fd = open(pathf,O_RDWR);

    if (fd < 0){
     printf("Open Failed!\n");
    }

    if (stat(pathf, &sbuf) == -1) {
    	printf("stat Failed!\n");
    }

    mapeo = mmap(0,sbuf.st_size,PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (mapeo == MAP_FAILED){
     printf("map Failed!\n");
    }

    GAdmin = (GAdm*) mapeo;
    GTNodo = GAdmin->admTnodo;
    GBitmap = bitarray_create(GAdmin->admbitmap, sbuf.st_size/BLOCK_SIZE/8);

    printf("File: %s\n",GTNodo[6].fname);


	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comunicarse con el kernel, delegar
    // en varios threads
	return fuse_main(argc,argv, &grasa_oper, NULL);


//    return EXIT_SUCCESS;

}
