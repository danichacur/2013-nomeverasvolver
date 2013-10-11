/*
* planificador.c
*
* Created on: 28/09/2013
* Author: utnso
*/

#include "orquestador.h"

#define PATH_CONFIG_PLA "../pla.conf"
/*
extern t_dictionary *bloqueados;
extern t_dictionary *listos;
extern t_dictionary *anormales;
extern t_dictionary *monitoreo;
*/
t_config *config;
t_log* logger;

//void planificador_analizar_mensaje(int32_t socket, enum tipo_paquete tipoMensaje, char* mensaje);


void *hilo_planificador(int32_t *nivel) {

	// int32_t miNivel = (int32_t) nivel;

	//inicialización
	config = config_create(PATH_CONFIG_PLA);
	char *PATH_LOG = config_get_string_value(config, "PATH_LOG_PLA");
	logger = log_create(PATH_LOG, "PLANIFICADOR", true, LOG_LEVEL_INFO);
	//inicialización

	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	// struct sockaddr_in remoteaddr; // dirección del cliente
	int fdmax = 0; // número máximo de descriptores de fichero
	// socklen_t addrlen;
	int i;
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);

	//todo ver dsp como ir metiendo aca los personajes para monitorear
	/* t_list *p_listos = dictionary_get(listos, miNivel);
	while (p_listos == NULL ) {
	;
	}
	*/
	//FD_SET(p_listos->personaje, &master);
	//fdmax = p_listos->personaje;
	//todo ver dsp como ir metiendo aca los personajes para monitorear

	// bucle principal
	for (;;) {
		read_fds = master;
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL ) == -1) {
			perror("select");
			exit(1);
		}

		//todo ver dsp como ir metiendo aca los personajes para monitorear
		//FD_SET(newfd, &master); // añadir al conjunto maestro
		//if (newfd > fdmax) { // actualizar el máximo
		// fdmax = newfd;
		//}
		//todo ver dsp como ir metiendo aca los personajes para monitorear

		// explorar conexiones existentes en busca de datos que leer
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				// ¡¡tenemos datos!! FD_ISSET comprueba si alguien mando algo!
				enum tipo_paquete tipoMensaje;
				char* mensaje = NULL;

				// gestionar datos del cliente del socket i!
				if (!recibirMensaje(i, &tipoMensaje, mensaje)) {

					//todo: borrarlo de bloqueados , listos, etc
					//eliminarlo de las estructuras

					close(i); // ¡Hasta luego!
					FD_CLR(i, &master); // eliminar del conjunto maestro
				} else {
					// tenemos datos del cliente del socket i!
					printf("Llego el tipo de paquete: %s ./n",
					nombre_del_enum_paquete(tipoMensaje));
					printf("Llego este mensaje: %s ./n", mensaje);

					// planificador_analizar_mensaje(i, tipoMensaje, mensaje);

				} // fin seccion recibir OK los datos
			} // fin de tenemos datos
		}

	} // fin bucle for principal

	pthread_exit(NULL);

}
