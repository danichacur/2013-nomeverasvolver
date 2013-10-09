/*
 * sockets.c
 *
 *  Created on: 06/10/2013
 *      Author: utnso
 */

#include "sockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <netdb.h>
#include <stdint.h>

#include <arpa/inet.h>
#include <string.h>

#define BUFF_SIZE 1024

int32_t crearSocketDeConexion(int32_t DIRECCION, int32_t PUERTO) {

	int32_t socketEscucha;
	struct sockaddr_in socketInfo;
	int32_t optval = 1;

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

	if (listen(socketEscucha, 1) != 0) {
		perror("Error al poner a escuchar socket");
		//	 pthread_exit(NULL );
	}

	return socketEscucha;
}

int32_t cliente_crearSocketDeConexion(char *DIRECCION, int32_t PUERTO) {

	int32_t unSocket;

	struct sockaddr_in socketInfo;

	//printf("Conectando...\n");

	// Crear un socket:
	// AF_INET: Socket de internet IPv4
	// SOCK_STREAM: Orientado a la conexion, TCP
	// 0: Usar protocolo por defecto para AF_INET-SOCK_STREAM: Protocolo TCP/IPv4
	if ((unSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error al crear socket");
		return -1;
	}

	socketInfo.sin_family = AF_INET;
	socketInfo.sin_addr.s_addr = inet_addr(DIRECCION);
	socketInfo.sin_port = htons(PUERTO);

	// Conectar el socket con la direccion 'socketInfo'.
	if (connect(unSocket, (struct sockaddr*) &socketInfo, sizeof(socketInfo))
			!= 0) {
		perror("Error al conectar socket");
		return -1;
	}
	return unSocket;
}

int32_t enviarMensaje(int32_t socket, enum tipo_paquete tipoMensaje,
		char* mensaje) {

	//Esto es lo que usaba Pablo A en el TP suyo para crear a variable que se enviaba y como iba concatenando
	//char buffer[BUFF_SIZE];
	//buffer[0] = 'E';
	//strcpy(&buffer[1],nom);

	uint32_t longitud = strlen(mensaje);
	char mensajeReal[BUFF_SIZE];
	strcpy(&mensajeReal[0], mensaje);

	struct t_cabecera miCabecera;
	miCabecera.tipoP = tipoMensaje;
	miCabecera.length = longitud;

	if (send(socket, (void *) &miCabecera, sizeof(struct t_cabecera), 0) < 0) {
		perror("enviarMensaje: Error al Enviar Header\n");
		return EXIT_FAILURE;
	} else {

		if (send(socket, mensaje, longitud + 1, 0) < 0) {
			perror("enviarMensaje: Error al Enviar Mensaje\n");
		}
	}

	return EXIT_SUCCESS;
}

int32_t recibirMensaje(int32_t socket, enum tipo_paquete *tipoMensaje,
		char* mensaje) {
	int nbytes;
	char MensajeEnvio[BUFF_SIZE];
	struct t_cabecera miCabecera;
	if ((nbytes = recv(socket, &miCabecera, sizeof(struct t_cabecera), 0))
			<= 0) {

		if (nbytes == 0) {
			// conexión cerrada
			printf("El cliente del socket %d se desconecto\n", socket);
		} else {
			perror("recibeMensaje: Error al Recibir Cabecera\n");
		}
		return EXIT_FAILURE;
	} else {
		if ((nbytes = recv(socket, MensajeEnvio, miCabecera.length, 0)) <= 0) {
			if (nbytes == 0) {
				// conexión cerrada
				printf("El cliente del socket %d se desconecto\n", socket);
			} else {
				perror("recibeMensaje: Error al Recibir Cabecera\n");
			}
			return EXIT_FAILURE; //Devuelve distinto de 0
		} else {
			(*tipoMensaje) = miCabecera.tipoP;
			mensaje = malloc(sizeof(char) * miCabecera.length + 1);
			strcpy(&MensajeEnvio[1], mensaje);
			return EXIT_SUCCESS; //Devuelve 0
		}
	}
}
