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
extern t_dictionary *anormales;*/
extern t_dictionary *monitoreo;

t_config *config;
t_log* logger;

void planificador_analizar_mensaje(int32_t socket, enum tipo_paquete tipoMensaje, char* mensaje);


void *hilo_planificador(int32_t *nivel) {

	int32_t miNivel = (int32_t) nivel;

	//inicialización
	config = config_create(PATH_CONFIG_PLA);
	char *PATH_LOG = config_get_string_value(config, "PATH_LOG_PLA");
	logger = log_create(PATH_LOG, "PLANIFICADOR", true, LOG_LEVEL_INFO);
	//inicialización

	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	int fdmax; // número máximo de descriptores de fichero
	int i;
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);

	//voy metiendo aca los personajes para monitorear
	char *str_nivel = string_from_format("%d", miNivel);
	t_list *p_monitoreo = dictionary_get(monitoreo, str_nivel);
	printf("por entrar al while infinito de espera de conexiones");
	while (list_is_empty(p_monitoreo)) {
		;
	}
	//todo - poner un semaforo aca!
	t_monitoreo *aux = list_remove(p_monitoreo, 0); //el primer elemento de la lista
	//todo - poner un semaforo aca!
	FD_SET(aux->fd, &master);
	fdmax = aux->fd;
	free(aux);
	//voy metiendo aca los personajes para monitorear

	// bucle principal
	for (;;) {
		read_fds = master;
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL ) == -1) {
			perror("select");
			exit(1);
		}

		//voy metiendo aca los personajes para monitorear
		int j = 0;
		while (!list_is_empty(p_monitoreo)){
			//todo - poner un semaforo aca!
			aux = list_remove(p_monitoreo, j);
			//todo - poner un semaforo aca!
			j++;
			FD_SET(aux->fd, &master); // añadir al conjunto maestro
			if (aux->fd > fdmax) { // actualizar el máximo
				fdmax = aux->fd;
			}
		}
		//voy metiendo aca los personajes para monitorear

		// explorar conexiones existentes en busca de datos que leer
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				// ¡¡tenemos datos!! FD_ISSET comprueba si alguien mando algo!
				enum tipo_paquete tipoMensaje;
				char* mensaje = NULL;

				// gestionar datos del cliente del socket i!
				if (!recibirMensaje(i, &tipoMensaje, mensaje)) {

					//todo: borrarlo de bloqueados , listos, etc y que libere los recursos!
					//eliminarlo de las estructuras

					close(i); // ¡Hasta luego!
					FD_CLR(i, &master); // eliminar del conjunto maestro
				} else {
					// tenemos datos del cliente del socket i!
					printf("Llego el tipo de paquete: %s ./n",
					nombre_del_enum_paquete(tipoMensaje));
					printf("Llego este mensaje: %s ./n", mensaje);

					planificador_analizar_mensaje(i, tipoMensaje, mensaje);

				} // fin seccion recibir OK los datos
			} // fin de tenemos datos
		}
	} // fin bucle for principal

	pthread_exit(NULL);

}

void planificar(void){

}
void tratamiento_muerte(void){

}

void planificador_analizar_mensaje(int32_t socket, enum tipo_paquete tipoMensaje,
char* mensaje) {
	switch (tipoMensaje) {

		case PER_dameUnTurno_PLA: {
			planificar();//todo
			break;
		}
		case PER_posCajaRecurso_PLA: {
//pasamanos al nivel, sin procesar nada
			break;
		}
		case PER_movimiento_PLA: {
//pasamanos al nivel, sin procesar nada
			break;
		}
		case PER_recurso_PLA: {
//pasamanos al nivel, lo paso de listos a bloqueados
			break;
		}
		case PER_nivelFinalizado_PLA: {
//pasamanos al nivel, lo saco de listos
			break;
		}
		case PER_meMori_PLA: {
			tratamiento_muerte();//todo
			break;
		}
		case NIV_posCaja_PLA: {
//pasamanos al personaje, sin procesar nada
			break;
		}
		case NIV_movimiento_PLA: {
//pasamanos al personaje, sin procesar nada
			break;
		}
		case NIV_recursoConcedido_PLA: {
//pasamanos al personaje, lo paso de bloqueados a listos
			break;
		}
		case NIV_cambiosConfiguracion_PLA: {

			break;
		}
		case NIV_enemigosAsesinaron_PLA: {

			break;
		}
		case NIV_perMuereInterbloqueo_PLA: {

			break;
		}

		default:
		printf("mensaje erroneo");
		break;
	}
	free(mensaje);
}
