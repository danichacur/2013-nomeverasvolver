////////////////////////////////////////////////////INCLUDES////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <temporal.h>
#include <collections/list.h>
#include <config.h>
#include <pthread.h>
//#include <tad_items.h>
//#include <nivel.h>
#include <curses.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>


////////////////////////////////////////////////////ESPACIO DE DEFINICIONES////////////////////////////////////////////////////

#define DIRECCION INADDR_ANY   //INADDR_ANY representa la direccion de cualquier interfaz conectada con la computadora
#define PUERTO 5000
#define BUFF_SIZE 1024

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



////////////////////////////////////////////////////ESPACIO DE FUNCIONES////////////////////////////////////////////////////
/* leerArchivoConfiguracion();
	config = config_create(ruta);

		quantum = config_get_int_value(config, "quantum");
		retardo_aux = config_get_int_value(config, "retardo");



enemigo(){
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





////////////////////////////////////////////////////PROGRAMA PRINCIPAL////////////////////////////////////////////////////
int main (){

	printf("hola mundo!!");
	/* printf("hola mundo!!");

	 *config = config_create(ruta);
	quantum = config_get_int_value(config, "quantum");
	printf("el quantum es %d",quantum);

	cargarArchivoDeConfiguracion();
	crearCajasRecursos();
	conectarAPlataforma(); // implica handShake - envia algoritmo
	crearHilosEnemigos();
	crearHiloInterbloqueo();
	crearHiloInotify();
	selectConPlataforma();
	 */
	return 0;
}

int pruebaSockets() {

	int socketEscucha, socketNuevaConexion;
	int nbytesRecibidos;

	struct sockaddr_in socketInfo;
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

// Escuchar nuevas conexiones entrantes.
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


