/*
* planificador.c
*
* Created on: 28/09/2013
* Author: utnso
*/

#include "planificador.h"

#define PATH_CONFIG_PLA "../pla.conf"

extern t_dictionary *bloqueados;
extern t_dictionary *listos;
//extern t_dictionary *anormales;
extern t_dictionary *monitoreo;

t_config *config;
t_log* logger;

void planificador_analizar_mensaje(int32_t socket, enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel );


void *hilo_planificador(t_niveles_sistema *nivel) {

	int32_t miNivel = nivel->nivel;
	int32_t miFd = nivel->fd;
	printf("hola, soy el planificador del nivel %d , mi fd es %d \n", miNivel, miFd);
	//inicialización
	//config = config_create(PATH_CONFIG_PLA);
	//char *PATH_LOG = config_get_string_value(config, "PATH_LOG_PLA");
	//logger = log_create(PATH_LOG, "PLANIFICADOR", true, LOG_LEVEL_INFO);
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
				if (recibirMensaje(i, &tipoMensaje, &mensaje)!= EXIT_SUCCESS) {

					//todo: borrarlo de bloqueados , listos, etc y que libere los recursos!
					//eliminarlo de las estructuras

					close(i); // ¡Hasta luego!
					FD_CLR(i, &master); // eliminar del conjunto maestro
				} else {
					// tenemos datos del cliente del socket i!
					printf("Llego el tipo de paquete: %s .\n",
					obtenerNombreEnum(tipoMensaje));
					printf("Llego este mensaje: %s .\n", mensaje);

					planificador_analizar_mensaje(i, tipoMensaje, mensaje, nivel );

				} // fin seccion recibir OK los datos
			} // fin de tenemos datos
		}
	} // fin bucle for principal

	pthread_exit(NULL);

}

void planificar(char * str_nivel){
	//todo : esto es solo para poder hacer las pruebas. hay que desarrollar los algoritmos
	t_list *p_listos = dictionary_get(listos, str_nivel);
	t_pers_por_nivel *aux = list_remove(p_listos, 0); //el primer elemento de la lista

	printf("ahora le toca a %c moverse", aux->personaje);
	enviarMensaje (aux->fd, PLA_turnoConcedido_PER, "0");

	list_add(p_listos, aux);

}
void tratamiento_muerte(int32_t socket, int32_t nivel_fd, char* mensaje, char* str_nivel){

	printf("le aviso al nivel que el personaje %c murio \n", mensaje[0]);
	printf("yo planificador debería liberar recursos? creo que no \n");
	enviarMensaje (nivel_fd, PLA_personajeMuerto_NIV , mensaje);

	t_list *p_listos = dictionary_get(listos, str_nivel);

	int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
		return nuevo->fd == socket;
	}
	t_pers_por_nivel *aux = list_remove_by_condition(p_listos, (void*) _esta_personaje); //el primer elemento de la lista


	printf("el personaje %c murio, se lo saca de este nivel %s", aux->personaje, str_nivel);
	free(aux);

}

void planificador_analizar_mensaje(int32_t socket, enum tipo_paquete tipoMensaje,
char* mensaje, t_niveles_sistema *nivel ) {
	char *str_nivel = string_from_format("%d", nivel->nivel);

	switch (tipoMensaje) {

	/*	case PER_dameUnTurno_PLA: {
			planificar(str_nivel);//todo
			break;
		}
		*/case PER_posCajaRecurso_PLA: {
			//pasamanos al nivel, sin procesar nada
			enviarMensaje (nivel->fd, PLA_posCaja_NIV , mensaje);
			enum tipo_paquete t_mensaje;
			char* m_mensaje = NULL;
			recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);
			//if (t_mensaje = NIV_posCaja_PLA){
			enviarMensaje (socket, PLA_posCajaRecurso_PER , m_mensaje);
			//}
			break;
		}
		case PER_movimiento_PLA: {
			//pasamanos al nivel, sin procesar nada
			enviarMensaje (nivel->fd, PLA_movimiento_NIV , mensaje);
			enum tipo_paquete t_mensaje;
			char* m_mensaje = NULL;
			recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);
			//if (t_mensaje = NIV_movimiento_PLA){
			enviarMensaje (socket, PLA_movimiento_PER , m_mensaje);
			//}
			break;
		}
		case PER_recurso_PLA: {
			//pasamanos al nivel, lo paso de listos a bloqueados
			t_list *p_listos = dictionary_get(listos, str_nivel);
			t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);

			int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
				return nuevo->fd == socket;
			}
			t_pers_por_nivel *aux = list_remove_by_condition(p_listos, (void*) _esta_personaje); //el primer elemento de la lista

			printf("se bloquea a %c por pedir un recurso", aux->personaje);
			list_add(p_bloqueados, aux);


			enviarMensaje (nivel->fd, PLA_solicitudRecurso_NIV , mensaje);
			enum tipo_paquete t_mensaje;
			char* m_mensaje = NULL;
			recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);
			if (t_mensaje == NIV_recursoConcedido_PLA){
				if(atoi(m_mensaje) == 0){//recurso concedido

					//
					t_pers_por_nivel *aux = list_remove_by_condition(p_bloqueados, (void*) _esta_personaje); //el primer elemento de la lista

					printf("se desbloquea a %c por haber obtenido su recurso", aux->personaje);
					list_add(p_listos, aux);
					//

					enviarMensaje (socket, PLA_rtaRecurso_PER , m_mensaje);

				}else
				printf("el personaje %c sigue bloqueado porque no le dieron el recurso", aux->personaje);
			}
			break;
		}
		case PER_nivelFinalizado_PLA: {
			//pasamanos al nivel, lo saco de listos
			t_list *p_listos = dictionary_get(listos, str_nivel);
			int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
				return nuevo->fd == socket;
			}
			t_pers_por_nivel *aux = list_remove_by_condition(p_listos, (void*) _esta_personaje); //el primer elemento de la lista
			printf("el personaje %c termino este nivel %d", aux->personaje, nivel->nivel);
			free(aux);

			enviarMensaje (nivel->fd, PLA_nivelFinalizado_NIV , mensaje);
			enum tipo_paquete t_mensaje;
			char* m_mensaje = NULL;
			recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);
			//if (t_mensaje = OK){
			enviarMensaje (socket, OK , m_mensaje);
			//}
			break;
		}
		case PER_meMori_PLA: {
			tratamiento_muerte(socket,nivel->fd,mensaje,str_nivel);//todo
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
