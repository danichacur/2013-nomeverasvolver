////////////////////////////////////////////////////BIBLIOTECAS ESTANDAR////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <unistd.h>
#include <string.h>

////////////////////////////////////////////////////BIBLIOTECAS COMMONS////////////////////////////////////////////////////
#include "nivel.h"

////////////////////////////////////////////////////ESPACIO DE DEFINICIONES////////////////////////////////////////////////////
#define DIRECCION "192.168.43.59"   //INADDR_ANY representa la direccion de cualquier interfaz conectada con la computadora
#define BUFF_SIZE 1024
#define RUTA "./config.cfg"
#define PUERTO 4000
#define CANT_NIVELES_MAXIMA 100

////////////////////////////////////////////////////ESPACIO DE VARIABLES GLOBALES////////////////////////////////////////////////////
t_log* logger;
int cantidadIntentosFallidos;
t_list *listaRecursosNivel; //GLOBAL, PERSONAJES SABEN DONDE ESTAN SUS OBJETIVOS
t_list *listaCajas;
bool listaRecursosVacia;
char* nombreNivel;
t_list* items;
t_list * listaDePersonajes;
////////////////////////////////////////////////////PROGRAMA PRINCIPAL////////////////////////////////////////////////////


int main (){
	listaRecursosNivel=list_create();
	items = list_create();
	listaDePersonajes = list_create();
	leerArchivoConfiguracion(); //TAMBIEN CONFIGURA LA LISTA DE RECURSOS POR NIVEL
	inicializarMapaNivel(listaRecursosNivel);
	int32_t socketDeEscucha=handshakeConPlataforma(); //SE CREA UN SOCKET NIVEL-PLATAFORMA DONDE RECIBE LOS MENSAJES POSTERIORMENTE
	while(1){
		mensajesConPlataforma(socketDeEscucha); //ACA ESCUCHO TODOS LOS MENSAJES EXCEPTO HANDSHAKE
	}
	eliminarEstructuras();
	return true;
}
	/*
	crearHilosEnemigos();
	crearHiloInterbloqueo();
	crearHiloInotify();
	return EXIT_SUCCESS;
}
*/

////////////////////////////////////////////////////ESPACIO DE FUNCIONES////////////////////////////////////////////////////
int leerArchivoConfiguracion(){
	//VOY A LEER EL ARCHIVO DE CONFIGURACION DE UN NIVEL//
	t_config *config = config_create(RUTA);
	printf("Voy a leer el nivel\n");
	char *nombre= config_get_string_value(config, "Nombre");
	nombreNivel=nombre;
	printf("Voy a leer los atributos de  %s \n",nombre);
	int quantum = config_get_int_value(config, "quantum");
	printf("Paso el quantum %d \n",quantum);

	int recovery = config_get_int_value(config, "Recovery");
	printf("Paso el recovery %d \n",recovery);

	int enemigos = config_get_int_value(config, "Enemigos");
	printf("Paso los enemigos %d \n",enemigos);
	long tiempoDeadlock = config_get_long_value(config, "TiempoChequeoDeadlock");
	printf("Paso el tiempo de deadlock %ld \n",tiempoDeadlock);
	long sleepEnemigos = config_get_long_value(config, "Sleep_Enemigos");
	printf("Paso el sleep %ld \n",sleepEnemigos);
	char* tipoAlgoritmo=config_get_string_value(config,"algoritmo");
	printf("Paso el algoritmo %s \n",tipoAlgoritmo);
	char *direccionIPyPuerto = config_get_string_value(config, "Plataforma");
	printf("Paso el puerto e IP %s \n",direccionIPyPuerto);
	int retardoAux = config_get_int_value(config, "retardo");
	printf("Paso el retardo %d \n",retardoAux);
	int retardo=retardoAux*1000;
	printf("Imprimo el retardo en milisegundos %d \n",retardo);
	// LEO LAS CAJAS DEL NIVEL //
	char litCaja[6]="Caja1\0";
	printf("Literal = [%s]\n",litCaja);
	bool ret = config_has_property(config, litCaja);
	printf("Encontre la %s = %d \n",litCaja,ret);
	printf("Voy a leer las cajas\n");
		while (ret == true){
			printf("Encontre la %s = %d \n",litCaja,ret);
			char **caja = config_get_array_value(config, litCaja);
			printf("detalles de la caja %s %s %s %s %s \n",caja[0], caja[1],caja[2],caja[3],caja[4]);
			crearCaja(caja);
			printf("Creé una caja\n");
			litCaja[4]++;
			printf("Le sumo uno al numero de la caja\n");
			ret = config_has_property(config, litCaja);
		}
		printf("el nivel es %d \n",nombre[5]); //CORREGIR, LO DA EN ASCII
	return EXIT_SUCCESS;
}

void crearCaja(char ** caja){ //CREA LA UNIDAD CAJA Y LA ENGANCHA EN LA LISTA DE RECURSOS DEL NIVEL
	tRecursosNivel *unaCaja=malloc(sizeof(tRecursosNivel));
	unaCaja->nombre=caja[0];
	unaCaja->simbolo=caja[1];
	unaCaja->instancias=caja[2];
	unaCaja->posX=caja[3];
	unaCaja->posY=caja[4];
	int cantElementos= list_add(listaRecursosNivel,unaCaja);
	printf("cantidad de elementos %d",cantElementos);
	free(unaCaja);
}

void inicializarMapaNivel(t_list* listaRecursos){
    /*t_list* items = list_create(); //LISTA DE ITEMS DEL MAPA (CAJAS PERSONAJES Y ENEMIGOS)
	int rows, cols; // TAMAÑO DEL MAPA

	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&rows, &cols);
	tRecursosNivel *unaCaja= list_get(listaRecursos, 0);
	while(unaCaja!=NULL){
		char* simbolo=unaCaja->simbolo;
		char* instancias=unaCaja->instancias;
		char* posX=unaCaja->posX;
		char* posY=unaCaja->posY;
		int posXint=atoi(posX);
		int posYint=atoi(posY);
		int instanciasInt=atoi(instancias);
		CrearCaja(items,*simbolo, posXint, posYint, instanciasInt);
	}
	nivel_gui_dibujar(items,nombreNivel );
*/
}

int32_t handshakeConPlataforma(){
	char *tiempo=temporal_get_string_time();
	printf("ahora un nivel va a conectarse a la plataforma a las %s \n",tiempo);
	puts(tiempo);
	int32_t socketEscucha= cliente_crearSocketDeConexion(DIRECCION,PUERTO);
	int32_t ok= enviarMensaje(socketEscucha, NIV_handshake_ORQ,"1");
	printf("%d \n",ok);
	return socketEscucha;
}

void mensajesConPlataforma(int32_t socketEscucha) {
	enum tipo_paquete unMensaje;
	char* elMensaje=NULL;
	recibirMensaje(socketEscucha, &unMensaje,&elMensaje);
	//if(respuesta){
		switch (unMensaje) {

			case PLA_movimiento_NIV: {//graficar y actualizar la lista
				char ** mens = string_split(elMensaje,",");

				ITEM_NIVEL * pers = buscarPersonajeLista(items, mens[0]);
				pers->posx = atoi(mens[1]);
				pers->posy = atoi(mens[2]);

				t_personaje * personaje = buscarPersonajeListaPersonajes(listaDePersonajes, mens[0]);
				personaje->posicion->posX = atoi(mens[1]);
				personaje->posicion->posY = atoi(mens[2]);

				//MoverPersonaje(t_list* items, elMensaje[0], , int y);
				printf("el personaje se movio %s",elMensaje);
				int32_t respuesta=enviarMensaje(socketEscucha,NIV_movimiento_PLA,"0"); //hay q validar q se mueva dentro del mapa...
				printf("%d",respuesta);
				break;
			}
			case PLA_personajeMuerto_NIV:{
				char id=elMensaje[0];
				printf("el personaje %d murio",id);
			//	BorrarItem(items,id);
				break;
			}
			case PLA_solicitudRecurso_NIV:{

				break;
			}
			case PLA_nuevoPersonaje_NIV:{
				t_posicion * posicion=posicion_create();
				char * simbolo = malloc(strlen(elMensaje)+1);
				strcpy(simbolo,elMensaje);
				t_personaje * personaje = personaje_create(simbolo,posicion);
				list_add(listaDePersonajes,personaje);

				ITEM_NIVEL * item = malloc(sizeof(ITEM_NIVEL));
				item->id = elMensaje[0];
				item->item_type = PERSONAJE_ITEM_TYPE;
				item->posx = 0;
				item->posy = 0;
				item->quantity = 0;
				list_add(items,item);

				printf("agregue un personaje nuevo a la lista");
				break;
						}
			case PLA_posCaja_NIV:{

				int32_t mensaje= enviarMensaje(socketEscucha, NIV_posCaja_PLA,"1,3");
				printf("envie posicion mensaje %d",mensaje);
				break;
			}
			case OK1:{
				printf("la conexion se hizo ok %s\n",elMensaje);
				break;
			}
			default:
				printf("%s \n","recibio cualquier cosa");
				break;
		}
	//}
		free(elMensaje);

}

ITEM_NIVEL * buscarPersonajeLista(t_list * lista, char * simbolo){
	ITEM_NIVEL * item;
	ITEM_NIVEL * unItem;
	bool encontrado = false;
	int i=0;
	while(i < list_size(lista) && !encontrado){
		unItem = list_get(lista,i);
		if(unItem->item_type == PERSONAJE_ITEM_TYPE)
			if (unItem->id == simbolo[0]){
				encontrado = true;
				item = unItem;
			}
		i++;
	}
	return item;
}

t_personaje * buscarPersonajeListaPersonajes(t_list * lista, char * simbolo){
	t_personaje * personaje;
	t_personaje * unPers;

	bool encontrado = false;
	int i=0;
	while(i < list_size(lista) && !encontrado){
		unPers = list_get(lista,i);
		if (strcmp(unPers->simbolo,simbolo) == 0){
			encontrado = true;
			personaje = unPers;
		}
		i++;
	}

	return personaje;
}
/*
void eliminarEstructuras(){

}


void buscaPersonajeCercano(){

};

void moverseAlternado(){

};

void actualizarUltimoMovimiento(){

};

void crearseASiMismo(){

};

void movermeEnL(){

};



void crearHiloInotify(){
	r2 = pthread_create(&thr2,NULL,&hilo_inotify,NULL);
	sprintf(buffer_log,"Se lanza el hilo para notificar cambios del quantum");
	log_info(logger, buffer_log);
}

int hilo_inotify(void) {
	char buffer[BUF_LEN];
	int quantum_aux;
	int ret;
	int i;

	// Al inicializar inotify este nos devuelve un descriptor de archivo
	int file_descriptor = inotify_init();
	if (file_descriptor < 0) {
		perror("inotify_init");
}

	// Creamos un monitor sobre un path indicando que eventos queremos escuchar
	///home/utnso/workspace/inotify/src
	int watch_descriptor = inotify_add_watch(file_descriptor, "./", IN_MODIFY | IN_CREATE | IN_DELETE);

	// El file descriptor creado por inotify, es el que recibe la información sobre los eventos ocurridos
	// para leer esta información el descriptor se lee como si fuera un archivo comun y corriente pero
	// la diferencia esta en que lo que leemos no es el contenido de un archivo sino la información
	// referente a los eventos ocurridos


	i = 0;
	while (1) {

	int length = read(file_descriptor, buffer, BUF_LEN);
	printf("BUFFER = [%s]\n",buffer);
	if (length < 0) {
		perror("read");
	}
	printf("DESPUES DE LEER\n");

	int offset = 0;

	// Luego del read buffer es un array de n posiciones donde cada posición contiene
	// un eventos ( inotify_event ) junto con el nombre de este.
	while (offset < length) {

		// El buffer es de tipo array de char, o array de bytes. Esto es porque como los
		// nombres pueden tener nombres mas cortos que 24 caracteres el tamaño va a ser menor
		// a sizeof( struct inotify_event ) + 24.
		struct inotify_event *event = (struct inotify_event *) &buffer[offset];

		// El campo "len" nos indica la longitud del tamaño del nombre
		if (event->len) {
			// Dentro de "mask" tenemos el evento que ocurrio y sobre donde ocurrio
			// sea un archivo o un directorio
			if (event->mask & IN_CREATE) {
				if (e
				vent->mask & IN_ISDIR) {
					printf("The directory %s was created.\n", event->name);
				} else {
					printf("The file %s was created.\n", event->name);
				}
			} else if (event->mask & IN_DELETE) {
				if (event->mask & IN_ISDIR) {
					printf("The directory %s was deleted.\n", event->name);
				} else {
					printf("The file %s was deleted.\n", event->name);
				}
			} else if (event->mask & IN_MODIFY) {
				if (event->mask & IN_ISDIR) {
					printf("The directory [%s] was modified.\n", event->name);

				} else {
					printf("The file [%s] was modified.\n", event->name);
					ret = strcmp(event->name,ruta);
					if (ret == 0) {
						if (i == 0){
							sleep(1);
							config_destroy(config);
							config = config_create(ruta);
							ret = config_keys_amount(config);
							retardo_aux = config_get_int_value(config, "retardo");
							quantum_aux = config_get_int_value(config,"quantum");
							printf("El nuevo quantum es: %d\n",quantum_aux);
							printf("El nuevo retardo es: %d\n",retardo_aux);
							pthread_mutex_lock( &mutex3 );
							quantum = quantum_aux;
							pthread_mutex_unlock( &mutex3 );
							i ++;
						}
						else i = 0;
					}
				}
			}
		}
		else {
			printf("Hay algo raro...\n");
			printf("event->len = %d",event->len);
		}
		offset += sizeof (struct inotify_event) + event->len;
	}
	}

	inotify_rm_watch(file_descriptor, watch_descriptor);
	close(file_descriptor);

	printf("SALGO DEL PROGRAMA\n");

	return EXIT_SUCCESS;
}


void enemigo(){
	crearseASiMismo(); //random, verifica que no se cree en el (0,0)

	while(1){
		if(hayPersonajeAtacable){
			buscaPersonajeCercano();
			moverseAlternado();
			actualizarUltimoMovimiento();

			if(estoyArribaPersonaje){
				avisarAlNivel();
			}
		}else{
			movermeEnL();
		}
	}
}
*/












