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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTACION DE FUNCIONES FUSE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//grasa_getattr

static uint32_t grasa_getattr(const char *path, struct stat *stbuf)
{
	int res=0;
	uint32_t i;

	//printf("grasa_getattr|path:%s\n",path);

	memset(stbuf, 0, sizeof(struct stat));

	i = obtenerNodo(path,0);

	if (i == 1024)
		return -ENOENT;

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

	//printf("grasa_open|path:%s\n",path);

	i = obtenerNodo(path,0);

	if (i == 1024)
		return -ENOENT;

	//if ((fi->flags & 3) != O_RDONLY)
	//	return -EACCES;

	return 0;
}

// - grasa_read -

static uint32_t grasa_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
        (void) fi;
        size_t aCopiar=0,aLeer=0;
        off_t off;
        tObNroBloque NroBloque;
        uint32_t i;

        i = obtenerNodo(path,0);

        //printf("grasa_read,size_fuse:%d,offset_fuse:%d,fsize_grasa:%d\n",(int)size,(int)offset,GTNodo[i].file_size);

        if (i == 1024)
                return -ENOENT;

        if (offset + size > GTNodo[i].file_size)
                size = GTNodo[i].file_size - offset;

        if (offset < GTNodo[i].file_size) {

        	aLeer = GTNodo[i].file_size;
        	off   = offset;

        	while(off < offset + size){

        		NroBloque = obtenerNroBloque(i,off);

        		if (aLeer >= BLOCK_SIZE){
        			aCopiar = BLOCK_SIZE - NroBloque.offsetDatos;
        		}
        		else{
        			aCopiar = aLeer;
        		}

        		memcpy(buf,obtenerDatos(NroBloque.BloqueDatos,NroBloque.offsetDatos),aCopiar);

        		buf += aCopiar;
        		aLeer-=aCopiar;
        		off += aCopiar;
            }
            buf -= size;
        } else
                size = 0;

        return size;
}

// - grasa_readdir -

static uint32_t grasa_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	(void) offset;
	(void) fi;
	uint32_t i=0,j=0,k=0,l=0;

	//printf("grasa_readdir|path:%s\n",path);

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
	uint32_t i,j=1024,k;
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

	//printf("grasa_mkdir|path:%s\n",path);

	if(strlen(path) > 71)
		return -ENAMETOOLONG;

	k = obtenerNodo(path,0);

	if(k != 1024)
		return -EEXIST;

	for (i=0; i < 1024; i++){
		if (GTNodo[i].state == 0){
			j=i;
			i=1024;
		}
	}

	if(j < 1024){

		pthread_mutex_lock( &mutex1 );
			GTNodo[j].parent_dir_block = 0;

			if(strcmp(dirname(strdup(path)),"/") != 0){
				k = obtenerNodo(dirname(strdup(path)),0);
				if(k == 1024) return -ENOENT;
				GTNodo[j].parent_dir_block = k+1;
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

	//printf("grasa_rmdir|path:%s\n",path);

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

		//bitarray_clean_bit(GBitmap,j+2);
	pthread_mutex_unlock( &mutex1 );

	return 0;
}

// - grasa_unlink -

static uint32_t grasa_unlink(const char *path){
	ptrGBloque i=0,j=0,k,entero,*blqindatos;
	size_t fsize;
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

	//printf("grasa_unlink|path:%s\n",path);

	k = obtenerNodo(path,0);

	if (k == 1024)
		return -ENOENT;

	if (GTNodo[k].state == 2 || GTNodo[k].state == 0)
		return -EFAULT;

	pthread_mutex_lock( &mutex1 );
		fsize = GTNodo[k].file_size;
		GTNodo[k].parent_dir_block = 0;
		GTNodo[k].file_size = 0;
		GTNodo[k].state = 0;
		GTNodo[k].c_date = 0;
		GTNodo[k].m_date = 0;

		entero = fsize/BLOCK_SIZE;
		if(fsize%BLOCK_SIZE && entero < GFILEBYTABLE )
			entero++;

		while (i < BLKINDIRECT && GTNodo[k].blk_indirect[i] != 0 ){
			j=0;
			while (j < GFILEBYTABLE && entero ){
		    	blqindatos = (ptrGBloque*)(mapeo + GTNodo[k].blk_indirect[i]*BLOCK_SIZE);
				if (bitarray_test_bit(GBitmap,*(blqindatos+j)))
					bitarray_clean_bit(GBitmap,*(blqindatos+j));
				*(blqindatos+j) = 0;
				j++;
				entero--;
			}
			if (bitarray_test_bit(GBitmap,GTNodo[k].blk_indirect[i]))
				bitarray_clean_bit(GBitmap,GTNodo[k].blk_indirect[i]);
			GTNodo[k].blk_indirect[i] = 0;
			i++;
		}

	pthread_mutex_unlock( &mutex1 );

	return 0;
}

// - grasa_truncate -

static uint32_t grasa_truncate(const char *path, off_t newsize){
	ptrGBloque i;
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

	//printf("grasa_truncate|path:%s\n",path);

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
	uint32_t i,j=1024,k;
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

	//printf("grasa_create|path:%s\n",path);

	if(strlen(path) > 71)
		return -ENAMETOOLONG;

	k = obtenerNodo(path,0);

	if(k != 1024)
		return -EEXIST;

	for (i=0; i < 1024; i++){
		if (GTNodo[i].state == 0){
			j=i;
			i=1024;
		}
	}


	if(j < 1024){
		pthread_mutex_lock( &mutex1 );
			GTNodo[j].parent_dir_block = 0;

			if (strcmp(dirname(strdup(path)),"/") != 0){
				k = obtenerNodo(dirname(strdup(path)),0);
				if(k == 1024) return -ENOENT;
				GTNodo[j].parent_dir_block = k+1;
			}

			strcpy(GTNodo[j].fname,basename(strdup(path)));
			GTNodo[j].file_size = 0;
			GTNodo[j].state = 1;
			GTNodo[j].c_date = (uint64_t)time(NULL);
			GTNodo[j].m_date = (uint64_t)time(NULL);

		pthread_mutex_unlock( &mutex1 );

		return 0;
	}


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
	uint32_t i,j;
	size_t aCopiar=0,aGrabar=0,off=0;
	tObNroBloque NroBloque;
	ptrGBloque *bloqueDatosIndirecto;
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

	//printf("grasa_write|path:%s,size:%d,off:%d\n",path,(int)size,(int)offset);

	i = obtenerNodo(path,0);

	if (i == 1024)
		return -ENOENT;

	if (offset + size > BLOCK_SIZE*BLKINDIRECT)
		return -ENOMEM;

	aGrabar = size;
	off = offset;

	pthread_mutex_lock( &mutex1 );

	while(off < offset + size){

		NroBloque = obtenerNroBloque(i,off);

		//printf("Nb:%d,Off:%d,ind:%d,ent:%d,cantbloqd:%d\n",(int)NroBloque.BloqueDatos,(int)NroBloque.offsetDatos,NroBloque.indirecto,NroBloque.entero,(int)cantBloquesDatos);

		if(NroBloque.BloqueDatos == 0){
			if (NroBloque.indirecto == 0 && NroBloque.entero == 0){
				for (j=1026; j < (cantBloquesDatos*8)-1; j++){
					if (!bitarray_test_bit(GBitmap,j)){
						bitarray_set_bit(GBitmap,j);
						GTNodo[i].blk_indirect[NroBloque.indirecto] = j;
						for (j=0; j < 1024; j++){
								bloqueDatosIndirecto = (ptrGBloque*)(mapeo + GTNodo[i].blk_indirect[NroBloque.indirecto]*BLOCK_SIZE);
								*(bloqueDatosIndirecto + j) = 0;
							}
						//printf("indirecto:%d,\n",(int)GTNodo[i].blk_indirect[NroBloque.indirecto]);
						break;
					}
				}
			}

			for (j=1026; j < (cantBloquesDatos*8)-1; j++){
				if (!bitarray_test_bit(GBitmap,j)){
					bitarray_set_bit(GBitmap,j);
					bloqueDatosIndirecto = (ptrGBloque*)(mapeo + GTNodo[i].blk_indirect[NroBloque.indirecto]*BLOCK_SIZE);
					*(bloqueDatosIndirecto + NroBloque.entero) = j;
					NroBloque.BloqueDatos = j;
					//printf("blqdatos:%d,\n",(int)*(bloqueDatosIndirecto + NroBloque.entero));
					break;
				}
			}
		}

		//printf("Nb:%d,Off:%d,agrabar:%d,acopiar:%d\n",(int)NroBloque.BloqueDatos,(int)NroBloque.offsetDatos,aGrabar,aCopiar);

		if (aGrabar >= BLOCK_SIZE){
			aCopiar = BLOCK_SIZE - NroBloque.offsetDatos;
		}
		else{
			aCopiar = aGrabar;
		}

		memcpy(obtenerDatos(NroBloque.BloqueDatos,NroBloque.offsetDatos),buf,aCopiar);

		//printf("sizebuf:%d,buf:%s,sizedat:%d,bufdat:%s\n",strlen(buf),buf,strlen(obtenerDatos(NroBloque.BloqueDatos,NroBloque.offsetDatos)),obtenerDatos(NroBloque.BloqueDatos,NroBloque.offsetDatos));

		buf += aCopiar;
		aGrabar-=aCopiar;
		off += aCopiar;
	}
	GTNodo[i].file_size = size + offset;
	GTNodo[i].m_date = (uint64_t)time(NULL);
	buf -=size + offset;

	pthread_mutex_unlock( &mutex1 );

	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FUNCIONES AUXILIARES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
						(strcmp(GTNodo[GTNodo[i].parent_dir_block-1].fname,dir3) == 0) &&
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
							strcmp(GTNodo[GTNodo[i].parent_dir_block-1].fname,dir3) == 0 &&
							GTNodo[GTNodo[i].parent_dir_block-1].state != 0 &&
							strcmp(GTNodo[GTNodo[GTNodo[i].parent_dir_block-1].parent_dir_block-1].fname,dir2) == 0 &&
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
									strcmp(GTNodo[GTNodo[GTNodo[GTNodo[i].parent_dir_block-1].parent_dir_block-1].parent_dir_block-1].fname,dir1) == 0 &&
									GTNodo[GTNodo[GTNodo[GTNodo[i].parent_dir_block-1].parent_dir_block-1].parent_dir_block-1].state != 0 &&
									GTNodo[GTNodo[GTNodo[GTNodo[i].parent_dir_block-1].parent_dir_block-1].parent_dir_block-1].parent_dir_block == 0){
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

	//Directorio raiz
	if (strcmp(dir1, "/") == 0  && strcmp(dir2, "/") == 0 && strcmp(dir3, "/") == 0 && strcmp(Filename, "/") == 0){
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
//Salida: offset del bloque de datos,bloque de datos, posicion array indirectos y posicion entera dentro del array de 1024.

tObNroBloque obtenerNroBloque(ptrGBloque NroNodo, off_t offsetArchivo){
	ptrGBloque entero,indirecto,*blqindatos;
    tObNroBloque admNroBloque;

    printf("obtenerNroBloque\n");

    entero = offsetArchivo / BLOCK_SIZE;
    indirecto = entero / GFILEBYTABLE;

    admNroBloque.offsetDatos = offsetArchivo % BLOCK_SIZE;

	admNroBloque.BloqueDatos = 0;
    admNroBloque.indirecto = indirecto;
    admNroBloque.entero= entero;
    printf("obtNroBlqfsize:%d\n",(int)GTNodo[NroNodo].file_size);

    if(GTNodo[NroNodo].file_size != 0){
    	blqindatos = (ptrGBloque*)(mapeo + GTNodo[NroNodo].blk_indirect[indirecto]*BLOCK_SIZE);
    	admNroBloque.BloqueDatos = *(blqindatos+entero);
    }

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

	path = "./Disco/disk.bin";

    printf("File: %s\n",path);

    fd = open(path,O_RDWR);

    if (fd < 0){
     printf("Open Failed!\n");
     return -ENOENT;
    }

    if (stat(path, &sbuf) == -1) {
    	printf("stat Failed!\n");
    	return -ENOENT;
    }

    mapeo = mmap(0,sbuf.st_size,PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (mapeo == MAP_FAILED){
     printf("map Failed!\n");
     return -ENOENT;
    }

    GAdmin = (GAdm*) mapeo;
    GTNodo = GAdmin->admTnodo;
    cantBloquesDatos = sbuf.st_size/BLOCK_SIZE/8;
    GBitmap = bitarray_create(GAdmin->admbitmap,cantBloquesDatos);

	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comunicarse con el kernel, delegar
    // en varios threads
	return fuse_main(argc,argv, &grasa_oper, NULL);

}
