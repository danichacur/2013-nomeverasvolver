/*
 * orquestador.c
 *
 *  Created on: 28/09/2013
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // aca esta memset

/* Librerias de Commons */
#include <log.h>
#include <config.h>

/* Esto es para el select() */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/* Esto es para las conexiones */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define DIRECCION INADDR_ANY   //INADDR_ANY representa la direccion de cualquier
//interfaz conectada con la computadora

#define IP "127.0.0.1"
#define PUERTO 4000
#define BUFF_SIZE 1024

int main() {

	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	struct sockaddr_in myaddr; // dirección del servidor
	struct sockaddr_in remoteaddr; // dirección del cliente
	int fdmax; // número máximo de descriptores de fichero
	int socketEscucha; // descriptor de socket a la escucha
	int newfd; // descriptor de socket de nueva conexión aceptada
	char buf[BUFF_SIZE]; // buffer para datos del cliente
	int nbytes;
	int yes = 1; // para setsockopt() SO_REUSEADDR
	socklen_t addrlen;
	int i, j;
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);


// Crear un socket:
// AF_INET: Socket de internet IPv4
// SOCK_STREAM: Orientado a la conexion, TCP
// 0: Usar protocolo por defecto para AF_INET-SOCK_STREAM: Protocolo TCP/IPv4
	if ((socketEscucha = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	// Hacer que el SO libere el puerto inmediatamente luego de cerrar el socket.
	// obviar el mensaje "address already in use" (la dirección ya se está 	usando)
	if (setsockopt(socketEscucha, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))
			== -1) {
		perror("setsockopt");
		exit(1);
	}

	// enlazar
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = DIRECCION;
	myaddr.sin_port = htons(PUERTO);
	memset(&(myaddr.sin_zero), '\0', 8);
	if (bind(socketEscucha, (struct sockaddr *) &myaddr, sizeof(myaddr))
			== -1) {
		perror("bind");
		exit(1);
	}

	// escuchar
	if (listen(socketEscucha, 10) == -1) {
		perror("listen");
		exit(1);
	}

	// añadir socketEscucha al conjunto maestro
	FD_SET(socketEscucha, &master);

	// seguir la pista del descriptor de fichero mayor
	fdmax = socketEscucha; // por ahora es éste porque es el primero y unico

	// bucle principal
	for (;;) {
		read_fds = master;
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL ) == -1) {
			perror("select");
			exit(1);
		}
		// explorar conexiones existentes en busca de datos que leer
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!! FD_ISSET comprueba si alguien mando algo!
				if (i == socketEscucha) {
					// si hay actividad en el socket escucha es xq hay nuevas conexiones a el
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(socketEscucha,
							(struct sockaddr *) &remoteaddr, &addrlen)) < 0) {
						perror("accept");
					} else {
						FD_SET(newfd, &master); // añadir al conjunto maestro
						if (newfd > fdmax) { // actualizar el máximo
							fdmax = newfd;
						}
						printf("selectserver: new connection from %s on "
								"socket %d\n", inet_ntoa(remoteaddr.sin_addr),
								newfd);
					}
				} else {
					// gestionar datos del cliente del socket i!
					if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
						// error o conexión cerrada por el cliente
						if (nbytes == 0) {
							// conexión cerrada
							printf("selectserver: socket %d hung up\n", i);
						} else {
							perror("recv");
						}
						close(i); // ¡Hasta luego!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					} else {
						// tenemos datos del cliente del socket i!

						printf("Llegaron estos datos: %s ./n", buf);
						/*for (j = 0; j <= fdmax; j++) {
							// ¡enviar a todoo el mundo!
							if (FD_ISSET(j, &master)) {
								// excepto al socketEscucha y a nosotros mismos
								if (j != socketEscucha && j != i) {
									if (send(j, buf, nbytes, 0) == -1) {
										perror("send");
									}
								}
							}
						} */ // fin FOr de enviar a todoo el mundo
					} // fin seccion recibir OK los datos
				} // fin gestionar datos que llegaron
			} // fin de tenemos datos
		} // fin exploración de datos nuevos
	} // fin bucle for principal

	return EXIT_SUCCESS;
}
