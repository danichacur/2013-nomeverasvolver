////////////////////////////////////////////////////BIBLIOTECAS ESTANDAR////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <unistd.h>
#include <string.h>

////////////////////////////////////////////////////BIBLIOTECAS COMMONS////////////////////////////////////////////////////
#include <sockets/sockets.h>
#include <sockets/mensajes.h>
#include <sockets/estructuras.h>
#include <commons/temporal.h>
#include <collections/list.h>
#include <commons/config.h>
#include <pthread.h>
#include "nivel.h"
#include <commons/log.h>
#include <tad_items.h>

////////////////////////////////////////////////////ESPACIO DE DEFINICIONES////////////////////////////////////////////////////
#define DIRECCION "192.168.0.60"   //INADDR_ANY representa la direccion de cualquier interfaz conectada con la computadora
#define BUFF_SIZE 1024
#define RUTA "/home/utnso/Escritorio/config.cfg"
#define PUERTO 4000
#define CANT_NIVELES_MAXIMA 100

////////////////////////////////////////////////////ESPACIO DE VARIABLES GLOBALES////////////////////////////////////////////////////
t_log* logger;
int cantidadIntentosFallidos;
t_list *listaRecursosNivel; //GLOBAL, PERSONAJES SABEN DONDE ESTAN SUS OBJETIVOS
t_list *listaCajas;
t_list *listaDeNivelesFinalizados;
bool listaRecursosVacia;

////////////////////////////////////////////////////PROGRAMA PRINCIPAL////////////////////////////////////////////////////
int main (){
	listaRecursosNivel=list_create();
	leerArchivoConfiguracion(); //TAMBIEN CONFIGURA LA LISTA DE RECURSOS POR NIVEL
	//inicializarMapaNivel(listaRecursosNivel);
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
/*
void inicializarMapaNivel(t_list* listaRecursos){
    t_list* items = list_create(); //LISTA DE ITEMS DEL MAPA (CAJAS PERSONAJES Y ENEMIGOS)
	int rows, cols; // TAMAÑO DEL MAPA

	int x = 1;
	int y = 1;
	int ex1 = 10, ey1 = 14;
	int ex2 = 20, ey2 = 3;
	tRecursosNivel *unaCaja= list_get(listaRecursos, 0);
	while(unaCaja!=NULL){
		unaCaja->posX
		unaCaja->nombre
		unaCaja->simbolo
		unaCaja->instancias
		unaCaja->posX
		unaCaja->posY

	}
	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&rows, &cols);
	CrearPersonaje(items, '#', x, y);

	CrearEnemigo(items, '1', ex1, ey1);
	CrearEnemigo(items, '2', ex2, ey2);

	CrearCaja(items, 'H', 26, 10, 5);
	CrearCaja(items, 'M', 8, 15, 3);
	CrearCaja(items, 'F', 19, 9, 2);

	nivel_gui_dibujar(items, "Test Chamber 04");

	MoverPersonaje(items, '1', ex1, ey1 );
	MoverPersonaje(items, '2', ex2, ey2 );

	MoverPersonaje(items, '#', x, y);

	if (   ((p == 26) && (q == 10)) || ((x == 26) && (y == 10)) ) {
		restarRecurso(items, 'H');
	}


	if((p == x) && (q == y)) {
		BorrarItem(items, '#'); //si chocan, borramos uno (!)
	}

	nivel_gui_dibujar(items, "Test Chamber 04");

	BorrarItem(items, '#');
	BorrarItem(items, '@');

	BorrarItem(items, '1');
	BorrarItem(items, '2');

	BorrarItem(items, 'H');
	BorrarItem(items, 'M');
	BorrarItem(items, 'F');

	nivel_gui_terminar();


}
*/
int32_t handshakeConPlataforma(){
	char *tiempo=temporal_get_string_time();
			printf("ahora un nivel va a conectarse a la plataforma a las %s \n",tiempo);
			puts(tiempo);
			int32_t socketEscucha= cliente_crearSocketDeConexion(DIRECCION,PUERTO);
			int32_t ok= enviarMensaje(socketEscucha, NIV_handshake_ORQ,"1");
			printf("%d \n",ok);
			enum tipo_paquete unMensaje;
			char* elMensaje=NULL;
			int32_t respuesta= recibirMensaje(socketEscucha, &unMensaje,&elMensaje); //recibo ok del orquestador por el handshake
			printf("la conexion se hizo ok %s\n",elMensaje);
			printf("%d",respuesta);
			free(elMensaje);
			int32_t respuesta1= recibirMensaje(socketEscucha, &unMensaje,&elMensaje); //recibo peticion de ubicacion de caja
			printf("%d",respuesta1);
			printf("requiero la caja de %s\n",elMensaje);
			free(elMensaje);
			int32_t ok1= enviarMensaje(socketEscucha, NIV_posCaja_PLA,"1,3");
			printf("%d",ok1);
			return socketEscucha;
}

void mensajesConPlataforma(int32_t socketEscucha) {
	enum tipo_paquete unMensaje;
	char* elMensaje=NULL;
	int32_t respuesta= recibirMensaje(socketEscucha, &unMensaje,&elMensaje);
	if(respuesta){
		switch (unMensaje) {

			case PLA_movimiento_NIV: {
				break;
			}
			case PLA_personajeMuerto_NIV:{
				break;
			}
			case PLA_solicitudRecurso_NIV:{
				break;
			}
			case PLA_posCaja_NIV:{
				break;
			}
			case NIV_handshake_ORQ:{

				break;
			}
			default: break;
		}
	}
}
void eliminarEstructuras(){

}
	/*

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












