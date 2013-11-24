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
	int res=0;
	uint32_t i;

	//printf("grasa_getattr\n");

	memset(stbuf, 0, sizeof(struct stat));

	i = obtenerNodo(path,0);

	if (i == 1024)
		return -ENOENT;

	//printf("path: %s\n",path);
	//printf("Nodo.name: %s\n",GTNodo[i].fname);
	//printf("Nodo.state: %d\n",GTNodo[i].state);

	if (GTNodo[i].state == 2) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (GTNodo[i].state == 1) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = GTNodo[i].file_size;
	}

	return res;
}

// - grasa_open -

static uint32_t grasa_open(const char *path, struct fuse_file_info *fi) {
	uint32_t i;

	printf("grasa_open\n");

	i = obtenerNodo(path,0);

	if (i == 1024)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

// - grasa_read -

static uint32_t grasa_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
        (void) fi;
        size_t aCopiar=0,aLeer=0;
        tObNroBloque NroBloque;
        uint32_t i;

        i = obtenerNodo(path,0);

        printf("grasa_read,size:%d,offset%d,fsize:%d\n",(int)size,(int)offset,GTNodo[i].file_size);

        if (i == 1024)
                return -ENOENT;

        if (offset + size > GTNodo[i].file_size)
                size = GTNodo[i].file_size - offset;

        if (offset <= GTNodo[i].file_size) {

        	aLeer = GTNodo[i].file_size;

        	while(offset < GTNodo[i].file_size){

        		NroBloque = obtenerNroBloque(i,offset);

        		if (aLeer >= BLOCK_SIZE){
        			aCopiar = BLOCK_SIZE - NroBloque.offsetDatos;
        		}
        		else{
        			aCopiar = aLeer;
        		}

        		printf("offset:%d,aCopiar:%d\n",(int)offset,(int)aCopiar);
        		printf("offsetDa:%d,nB:%d\n",(int)NroBloque.offsetDatos,(int)NroBloque.BloqueDatos);

        		memcpy(&buf[offset],obtenerDatos(NroBloque.BloqueDatos,NroBloque.offsetDatos),aCopiar);

        		//buf += aCopiar;
        		aLeer-=aCopiar;
        		offset += aCopiar;
            }
            //TODO ver si usar size o filesize
            buf -= GTNodo[i].file_size;
            size = GTNodo[i].file_size;
        } else
                size = 0;

        return size;
}

// - grasa_readdir -

static uint32_t grasa_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	(void) offset;
	(void) fi;
	uint32_t i=0,j=0,k=0,l=0;

	//printf("grasa_readdir\n");

	// "." y ".." son entradas validas, la primera es una referencia al directorio donde estamos parados
	// y la segunda indica el directorio padre
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	k = obtenerNodo(path,i);

	while (i < 1024){
		l = obtenerNodoRelacionado(path,i,k);
		if (l != 1024){
			filler(buf,GTNodo[l].fname, NULL, 0);
			i = l;
			//printf("filedir:%s,i:%d,Nn:%d\n",GTNodo[l].fname,i,l);
			j = 1;
		}
		i++;
	}

	if(j==0)
		return -ENOENT;

	return 0;
}

// - grasa_mkdir -

static uint32_t grasa_mkdir(const char *path, mode_t mode){
	uint32_t i,j=2,k;
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

	printf("grasa_mkdir\n");

	if(strlen(path) > 71)
		return -ENAMETOOLONG;

	for (i=2; i < 1025; i++){
		j=i;
		i=1025;
		if (bitarray_test_bit(GBitmap,j)){
			printf("i:%d,j:%d,\n",(int)i,(int)j);
			i=j;
		}
	}

	printf("jota:%d,\n",(int)j);

	if(j < 1025){

		pthread_mutex_lock( &mutex1 );
			GTNodo[j].parent_dir_block = 0;

			if(strcmp(dirname(strdup(path)),"/") != 0){
				k = obtenerNodo(path,0);
				GTNodo[j].parent_dir_block = GTNodo[k].parent_dir_block;
			}

			printf("dirblk:%d,\n",(int)GTNodo[j].parent_dir_block);

			strcpy(GTNodo[j].fname,basename(strdup(path)));
			GTNodo[j].file_size = 0;
			GTNodo[j].state = 2;
			GTNodo[j].c_date = (uint64_t)time(NULL);
			GTNodo[j].m_date = (uint64_t)time(NULL);

			bitarray_set_bit(GBitmap,j);
		pthread_mutex_unlock( &mutex1 );

		return 0;
	}


	return -ENOSPC;
}

// - grasa_rmdir -

static uint32_t grasa_rmdir(const char *path){
	uint32_t i,j;
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

	printf("grasa_rmdir\n");

	j = obtenerNodo(path,0);

	if (j == 1024)
		return -ENOENT;

	if (GTNodo[j].state < 2)
		return -ENOTDIR;

	for(i=0;i < 1024;i++){
		if(GTNodo[i].parent_dir_block == j)
			return -ENOTEMPTY;
	}

	pthread_mutex_lock( &mutex1 );
		GTNodo[j].parent_dir_block = 0;
		GTNodo[j].file_size = 0;
		GTNodo[j].state = 0;
		GTNodo[j].c_date = 0;
		GTNodo[j].m_date = 0;

		bitarray_clean_bit(GBitmap,j+2);
	pthread_mutex_unlock( &mutex1 );

	return 0;
}

// - grasa_unlink -

static uint32_t grasa_unlink(const char *path){
	ptrGBloque i=0,j=0,k;
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

	printf("grasa_unlink\n");

	k = obtenerNodo(path,0);

	if (k == 1024)
		return -ENOENT;

	if (GTNodo[k].state == 2 || GTNodo[k].state == 0)
		return -EFAULT;

	pthread_mutex_lock( &mutex1 );
		GTNodo[k].parent_dir_block = 0;
		GTNodo[k].file_size = 0;
		GTNodo[k].state = 0;
		GTNodo[k].c_date = 0;
		GTNodo[k].m_date = 0;

		//Liberar inodo, se coloca +2 ya que el NroNodo es relativo a la tabla de Nodos
		bitarray_clean_bit(GBitmap,k+2);

		j = GTNodo[k].blk_indirect[i];
		while (bitarray_test_bit(GBitmap,j) && i > 1024){
			bitarray_clean_bit(GBitmap,j);
			i++;
			j = GTNodo[k].blk_indirect[i];
		}
	pthread_mutex_unlock( &mutex1 );

	return 0;
}

// - grasa_truncate -

static uint32_t grasa_truncate(const char *path, off_t newsize){
	ptrGBloque i;
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

	printf("grasa_truncate\n");

	i = obtenerNodo(path,0);

	if (i == 1024)
		return -ENOENT;

	if (GTNodo[i].state == 2 || GTNodo[i].state == 0)
		return -EFAULT;

	pthread_mutex_lock( &mutex1 );
		GTNodo[i].file_size = newsize;
		GTNodo[i].m_date =  (uint64_t)time(NULL);
	pthread_mutex_unlock( &mutex1 );

	return 0;
}

static uint32_t grasa_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	(void) fi;
	off_t i,j=2;
	uint32_t k;
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

	printf("grasa_create\n");

	if(strlen(path) > 71)
		return -ENAMETOOLONG;

	pthread_mutex_lock( &mutex1 );
		for (i=2; i < 1026; i++){
			j=i;
			i=1026;
			if (bitarray_test_bit(GBitmap,j))
				i=j;
		}

		printf("j:%d\n", (int)j);

	if(j < 1026){

		GTNodo[j].parent_dir_block = 0;

		if (strcmp(dirname(strdup(path)),"/") != 0){
			k = obtenerNodo(path,0);
			GTNodo[j].parent_dir_block = GTNodo[k].parent_dir_block;
		}

		strcpy(GTNodo[j].fname,basename(strdup(path)));
		GTNodo[j].file_size = 0;
		GTNodo[j].state = 1;
		GTNodo[j].c_date = (uint64_t)time(NULL);
		GTNodo[j].m_date = (uint64_t)time(NULL);

		bitarray_set_bit(GBitmap,j);
		pthread_mutex_unlock( &mutex1 );

		return 0;
	}
	pthread_mutex_unlock( &mutex1 );

	return -ENOENT;
}

// - grasa_write -

static uint32_t grasa_utimens(const char* path, const struct timespec ts[2]){
	return 0;
}

// - grasa_write -

static uint32_t grasa_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	(void) fi;
	(void) offset;
	uint32_t i;
	size_t aCopiar=0;
	tObNroBloque NroBloque;
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

	printf("grasa_write\n");

	i = obtenerNodo(path,0);

	if (i == 1024)
		return -ENOENT;

	if (offset + size > BLOCK_SIZE*BLKINDIRECT)
		return 0;

	pthread_mutex_lock( &mutex1 );

	while(offset <= offset + size){

		NroBloque = obtenerNroBloque(i,offset);

		if (bitarray_test_bit(GBitmap,NroBloque.BloqueDatos))
				bitarray_set_bit(GBitmap,NroBloque.BloqueDatos);

		aCopiar = BLOCK_SIZE - NroBloque.offsetDatos;

		memcpy(obtenerDatos(NroBloque.BloqueDatos,NroBloque.offsetDatos),buf,aCopiar);

		buf += aCopiar;
		offset += aCopiar;
	}
	//TODO ver si usar size o filesize
	GTNodo[i].file_size = offset + size;
	GTNodo[i].m_date = (uint64_t)time(NULL);
	buf -= size;

	pthread_mutex_unlock( &mutex1 );

	return size;
}

//FUNCIONES AUXILIARES

// - obtenerNodo -
//Entrada: Dado el path y el numero de inodo a partir del cual se quiere buscar (consecutivo)
//Salida: Devuelve el nro de inodo.

ptrGBloque obtenerNodo( const char *path, ptrGBloque j){
	ptrGBloque i;
	char *Filename,*dir1,*dir2,*dir3;

	//printf("obtenerNodo:%s\n",path);

	Filename = basename(strdup(path));
	dir1 = strdup(path);
	dir1 = dirname(dir1);
	dir1 = dirname(dir1);
	dir1 = dirname(dir1);
	dir1 = basename(dir1);
	dir2 = strdup(path);
	dir2 = dirname(dir2);
	dir2 = dirname(dir2);
	dir2 = basename(dir2);
	dir3 = strdup(path);
	dir3 = dirname(dir3);
	dir3 = basename(dir3);

	//printf("dir1:%s\n",dir1);
	//printf("dir2:%s\n",dir2);
	//printf("filename:%s\n",Filename);

	//Directorio raiz
	if (strcmp(dir1, "/") == 0  && strcmp(dir2, "/") == 0 && strcmp(dir3, "/") == 0 && strcmp(Filename, "/") == 0){
		for (i = j; i < 1024; i++){
			if (GTNodo[i].parent_dir_block == 0 &&
				GTNodo[i].state != 0){
				//printf("0 pdirblk: %d, posi:%d, posj:%d\n",GTNodo[i].parent_dir_block,i,j);
				return i;
			}
		}
	}
	else//Si tengo un solo nivel
		if (strcmp(dir1, "/") == 0  && strcmp(dir2, "/") == 0 && strcmp(dir3, "/") == 0 && strcmp(Filename, "/") != 0){
			for (i = j; i < 1024; i++){
				if (strcmp(GTNodo[i].fname,Filename) == 0 &&
					GTNodo[i].parent_dir_block == 0 &&
					GTNodo[i].state != 0){
					//printf("1 NodoBuscado: %s, posi: %d\n",Filename,i);
					return i;
				}
			}
		}
		else//Si tengo un solo nivel de directorio
			if (strcmp(dir1, "/") == 0  && strcmp(dir2, "/") == 0 && strcmp(dir3, "/") != 0){
				for (i = j; i < 1024; i++){
					if ((strcmp(GTNodo[i].fname,Filename) == 0)&&
						(strcmp(GTNodo[GTNodo[i].parent_dir_block-1].fname,dir2) == 0) &&
						(GTNodo[GTNodo[i].parent_dir_block-1].parent_dir_block == 0) &&
						(GTNodo[i].state != 0)){
						//printf("2 NodoBuscado: %s, posi: %d\n",Filename,i);
						return i;
					}
				}
			}
			else//Si tengo dos niveles de directorios
				if (strcmp(dir1, "/") == 0  && strcmp(dir2, "/") != 0){
					for (i = j; i < 1024; i++){
						if (strcmp(GTNodo[i].fname,Filename) == 0 &&
							GTNodo[i].state != 0 &&
							strcmp(GTNodo[GTNodo[i].parent_dir_block-1].fname,dir2) == 0 &&
							GTNodo[GTNodo[i].parent_dir_block-1].state != 0 &&
							strcmp(GTNodo[GTNodo[GTNodo[i].parent_dir_block-1].parent_dir_block-1].fname,dir1) == 0 &&
							GTNodo[GTNodo[GTNodo[i].parent_dir_block-1].parent_dir_block-1].parent_dir_block == 0 &&
							GTNodo[GTNodo[GTNodo[i].parent_dir_block-1].parent_dir_block-1].state != 0){
							//printf("3 NodoBuscado: %s, posi: %d\n",Filename,i);
							return i;
						}
					}
				}
					else
						if (strcmp(dir1, "/") != 0){
							for (i = j; i < 1024; i++){
								if (strcmp(GTNodo[i].fname,Filename) == 0 &&
									GTNodo[i].state != 0 &&
									strcmp(GTNodo[GTNodo[i].parent_dir_block-1].fname,dir3) == 0 &&
									GTNodo[GTNodo[i].parent_dir_block-1].state != 0 &&
									strcmp(GTNodo[GTNodo[GTNodo[i].parent_dir_block-1].parent_dir_block-1].fname,dir2) == 0 &&
									GTNodo[GTNodo[GTNodo[i].parent_dir_block-1].parent_dir_block-1].state != 0 &&
									strcmp(GTNodo[GTNodo[GTNodo[GTNodo[i].parent_dir_block-1].parent_dir_block-1].parent_dir_block].fname,dir1) == 0 &&
									GTNodo[GTNodo[GTNodo[GTNodo[i].parent_dir_block-1].parent_dir_block-1].parent_dir_block].state != 0 &&
									GTNodo[GTNodo[GTNodo[GTNodo[i].parent_dir_block-1].parent_dir_block-1].parent_dir_block].parent_dir_block == 0){
									//printf("3 NodoBuscado: %s, posi: %d\n",Filename,i);
									return i;
								}
							}
						}



	//Si no se ecuentra nodo devuelve 1024, validar este retorno.
	return 1024;
}

// - obtenerNodoRelacionado -
//Entrada: Dado el path, el numero de inodo a partir del cual se quiere buscar (consecutivo) y a cual esta relacionado
//Salida: Devuelve el nro de inodo relacionado.

ptrGBloque obtenerNodoRelacionado( const char *path, ptrGBloque j, ptrGBloque k){
	ptrGBloque i;
	char *Filename,*dir1,*dir2;

	//printf("obtenerNodo:%s\n",path);

	Filename = basename(strdup(path));
	dir1 = strdup(path);
	dir1 = dirname(dir1);
	dir1 = dirname(dir1);
	dir2 = strdup(path);
	dir2 = dirname(dir2);
	dir2 = basename(dir2);

//	printf("dir1:%s\n",dir1);
//	printf("dir2:%s\n",dir2);
//	printf("filename:%s\n",Filename);

	//Directorio raiz
	if (strcmp(dir1, "/") == 0  && strcmp(dir2, "/") == 0 && strcmp(Filename, "/") == 0){
		for (i = j; i < 1024; i++){
			if (GTNodo[i].parent_dir_block == 0 &&
				GTNodo[i].state != 0){
				//printf("r0 filename: %s, posi:%d, posj:%d\n",GTNodo[i].fname,i,j);
				return i;
			}
		}
	}
	else{//cualquier otro
			for (i = j; i < 1024; i++){
				if (GTNodo[i].parent_dir_block-1 == k &&
					GTNodo[i].state != 0){
					//printf("r1 NodoBuscado: %s, posi: %d, posj:%d\n",GTNodo[i].fname,i,j);
					return i;
				}
			}
		}

	//Si no se ecuentra nodo devuelve 1024, validar este retorno.
	return 1024;
}

// - obtenerNroBloque -
//Entrada: dado el nro de inodo y un offset del archivo a procesar.
//Salida: offset del bloque de datos y bloque de datos.

tObNroBloque obtenerNroBloque(ptrGBloque NroNodo, off_t offsetArchivo){
        off_t entero,indirecto;
        tObNroBloque admNroBloque;

        printf("obtenerNroBloque\n");

        entero = offsetArchivo / BLOCK_SIZE;
        indirecto = entero / GFILEBYTABLE;

        //Se calcula el resto y se resta 1 para contar desde 0.
        admNroBloque.offsetDatos = offsetArchivo % BLOCK_SIZE;
        //if (entero) --admNroBloque.offsetDatos;

    blqindatos = (ptrGBloque*)(mapeo + GTNodo[NroNodo].blk_indirect[indirecto]*BLOCK_SIZE);

    admNroBloque.BloqueDatos = *(blqindatos+entero);

        return admNroBloque;

}

// - obtenerDatos -
//Entrada: dado el nro de bloque de datos y un offset del archivo bloque de datos.
//Salida: datos desde el offset de entrada.

char * obtenerDatos(ptrGBloque NroBloqueDatos, off_t offsetbloque){

	printf("obtenerDatos\n");

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
		.utimens = grasa_utimens,
		.create = grasa_create,
};

//////////////////////////////////////////////MAIN////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
	int fd;
	struct stat sbuf;
	char *path;
	//char pathf[71];

	path = "/home/utnso/Escritorio/tmp/disk.bin";

	//memcpy (pathf,argv[1],strlen(argv[1]));
	//pathf[strlen(argv[1])] = '\0';
	//strcat(path,pathm);

    printf("File: %s\n",path);

    fd = open(path,O_RDWR);

    if (fd < 0){
     printf("Open Failed!\n");
    }

    if (stat(path, &sbuf) == -1) {
    	printf("stat Failed!\n");
    }

    mapeo = mmap(0,sbuf.st_size,PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (mapeo == MAP_FAILED){
     printf("map Failed!\n");
    }

    GAdmin = (GAdm*) mapeo;
    GTNodo = GAdmin->admTnodo;
    GBitmap = bitarray_create(GAdmin->admbitmap, sbuf.st_size/BLOCK_SIZE/8);

//    printf("File0: %s\n",GTNodo[0].fname);
    //    printf("File: %d\n",GTNodo[0].parent_dir_block);
    //printf("File1: %s\n",GTNodo[1].fname);
    //printf("File: %d\n",GTNodo[1].parent_dir_block);
    //printf("File2: %s\n",GTNodo[2].fname);
    //printf("File: %d\n",GTNodo[2].parent_dir_block);
    //printf("File3: %s\n",GTNodo[3].fname);
    //printf("File: %d\n",GTNodo[3].parent_dir_block);
    //printf("File4: %s\n",GTNodo[4].fname);
    //printf("File: %d\n",GTNodo[4].parent_dir_block);
    //printf("File5: %s\n",GTNodo[5].fname);
    //printf("File: %d\n",GTNodo[5].parent_dir_block);
    //printf("File6: %s\n",GTNodo[6].fname);
    //printf("File: %d\n",GTNodo[6].parent_dir_block);
    //printf("File7: %s\n",GTNodo[7].fname);
    //printf("File: %d\n",GTNodo[7].parent_dir_block);
    //printf("File8: %s\n",GTNodo[8].fname);
    //printf("File: %d\n",GTNodo[8].parent_dir_block);
    //printf("File9: %s\n",GTNodo[9].fname);
    //printf("File: %d\n",GTNodo[9].parent_dir_block);
    //printf("File10: %s\n",GTNodo[10].fname);
    //printf("File: %d\n",GTNodo[10].parent_dir_block);
    //printf("File11: %s\n",GTNodo[11].fname);
    //printf("File: %d\n",GTNodo[11].parent_dir_block);
    //printf("File12: %s\n",GTNodo[12].fname);
    //printf("File: %d\n",GTNodo[12].parent_dir_block);
    //printf("File13: %s\n",GTNodo[13].fname);
    //printf("File: %d\n",GTNodo[13].parent_dir_block);
    //printf("File14: %s\n",GTNodo[14].fname);
    //printf("File: %d\n",GTNodo[14].parent_dir_block);


	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comunicarse con el kernel, delegar
    // en varios threads
	return fuse_main(argc,argv, &grasa_oper, NULL);

}
