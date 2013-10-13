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
#include <temporal.h>
#include <collections/list.h>
#include <config.h>
#include <pthread.h>
//#include <tad_items.h>
//#include <nivel.h>


////////////////////////////////////////////////////ESPACIO DE DEFINICIONES////////////////////////////////////////////////////

#define DIRECCION "192.168.0.60"   //INADDR_ANY representa la direccion de cualquier interfaz conectada con la computadora
#define BUFF_SIZE 1024

#define PUERTO 4000
////////////////////////////////////////////////////ESPACIO DE VARIABLES////////////////////////////////////////////////////

char *ruta = "./configNivel.cfg";
t_config *config;
int quantum;
long int tiempoDeadlock;
int recovery;
int enemigos;
long int sleepEnemigos;
char * tipoAlgoritmo;
long int retardo;
char **caja;
char litCaja[6] = "Caja1\0";
char * direccionIPyPuerto;
////////////////////////////////////////////////////ESPACIO DE FUNCIONES////////////////////////////////////////////////////
/*
int leerArchivoConfiguracion();
	longint retardo_aux;
	config = config_create(ruta);
	quantum = config_get_int_value(config, "quantum");
	tiempoDeadlock = config_get_long_int_value(config, "TiempoChequeoDeadlock");
	sleepEnemigos = config_get_long_int_value(config, "Sleep_Enemigos");
	tipoAlgoritmo=config_get_string_value(config,"algortimo");
	direccionIPyPuerto = config_get_int_value(config, "Plataforma");
	retardo_aux = config_get_int_value(config, "retardo");
	retardo=retardo_aux*1000
	// Leo las cajas
		printf("Literal = [%s]\n",litCaja);
		ret = config_has_property(config, litCaja);
		printf("Paso el 1er ret\n");
		printf("Voy a leer las cajas\n");
		while (ret == true){
			caja = config_get_array_value(config, litCaja);
			crearCaja(&ListaItems,caja[1],caja[3],caja[4],caja[2],caja[5]));
			printf("Creé una caja\n");
			litCaja[4]++;
			printf("Le sumo uno al numero de la caja\n");
			ret = config_has_property(config, litCaja);

		return EXIT_SUCCESS;
		};

void crearCaja(t_listaItems *lista ,char ** elemento1,char ** elemento2,char ** elemento3,char ** elemento4,char ** elemento5){


}



void crearHiloInotify(){
        r2 = pthread_create(&thr2,NULL,&hilo_inotify,NULL);
		sprintf(buffer_log,"Se lanza el hilo para notificar cambios del quantum");
		log_info(logger, buffer_log);

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
int conectarmeConPlataforma() {

	int socketEscucha, socketNuevaConexion;
	int nbytesRecibidos;

	struct sockaddr_in socketInfo;  //aca indicarle la del orquestador y la mia
	char buffer[BUFF_SIZE];
	int optval = 1;

	// Crear un socket:
	// AF_INET: Socket de internet IPv4
	// SOCK_STREAM: Orientado a la conexion, TCP
	// 0: Usar protocolo por defecto para AF_INET-SOCK_STREAM: Protocolo TCP/IPv4
	if ((socketEscucha = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	// Hacer que el SO libere el puerto inmediatamente luego de cerrar el socket.
	setsockopt(socketEscucha, SOL_SOCKET, SO_REUSEADDR, &optval,
			sizeof(optval));

	socketInfo.sin_family = AF_INET;
	socketInfo.sin_addr.s_addr = DIRECCION; //Notar que aca no se usa inet_addr()
	socketInfo.sin_port = htons(PUERTO);

// Vincular el socket con una direccion de red almacenada en 'socketInfo'.
	if (bind(socketEscucha, (struct sockaddr*) &socketInfo, sizeof(socketInfo))
			!= 0) {

		perror("Error al bindear socket escucha");
		return EXIT_FAILURE;
	}

// Escuchar nuevas conexiones entrantes.El 10 es la cantidad de conexiones en cola
	if (listen(socketEscucha, 10) != 0) {

		perror("Error al poner a escuchar socket");
		return EXIT_FAILURE;

	}

	printf("Escuchando conexiones entrantes.\n");

// Aceptar una nueva conexion entrante. Se genera un nuevo socket con la nueva conexion.
	if ((socketNuevaConexion = accept(socketEscucha, NULL, 0)) < 0) {

		perror("Error al aceptar conexion entrante");
		return EXIT_FAILURE;

	}

	while (1) {

		// Recibir hasta BUFF_SIZE datos y almacenarlos en 'buffer'.
		if ((nbytesRecibidos = recv(socketNuevaConexion, buffer, BUFF_SIZE, 0))
				> 0) {

			printf("Mensaje recibido: ");
			fwrite(buffer, 1, nbytesRecibidos, stdout);
			printf("\n");
			printf("Tamanio del buffer %d bytes!\n", nbytesRecibidos);
			fflush(stdout);

			if (memcmp(buffer, "fin", nbytesRecibidos) == 0) {

				printf("Server cerrado correctamente.\n");
				break;

			}

		} else {
			perror("Error al recibir datos");
			break;
		}
	}

	close(socketEscucha);
	close(socketNuevaConexion);

	return EXIT_SUCCESS;
}


*/


////////////////////////////////////////////////////PROGRAMA PRINCIPAL////////////////////////////////////////////////////
int main (){

	printf("hola mundo!!\n");
	char *tiempo=temporal_get_string_time();
	puts(tiempo);
	int32_t unSocket= cliente_crearSocketDeConexion(DIRECCION,PUERTO);
	int32_t ok= enviarMensaje(unSocket, NIV_handshake_ORQ,"holaa");
	printf("%d",ok);
	enum tipo_paquete unMensaje;
	char* elMensaje=NULL;
	int32_t respuesta= recibirMensaje(unSocket, &unMensaje,&elMensaje);
	printf("%d\n",respuesta);
	printf("%s\n",elMensaje);
	printf("%s",nombre_del_enum_paquete(unMensaje));



	/* cargarArchivoDeConfiguracion();
	crearCajasRecursos();
	conectarmeConPlataforma(); // implica handShake - envia algoritmo
	crearHilosEnemigos();
	crearHiloInterbloqueo();
	crearHiloInotify();
*/
	return EXIT_SUCCESS;
}





