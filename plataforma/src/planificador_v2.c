/*
 * planificador_v2.c
 *
 *  Created on: 26/10/2013
 *      Author: utnso
 */

#include "planificador.h"

#define PATH_CONFIG_PLA "../pla.conf"

extern t_dictionary *bloqueados;
extern t_dictionary *listos;
//extern t_dictionary *anormales;
extern t_dictionary *monitoreo;

t_config *config;
t_log* logger;

void planificador_analizar_mensaje(t_pers_por_nivel *personaje,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel);
t_pers_por_nivel *planificar(char * str_nivel);

void *hilo_planificador(t_niveles_sistema *nivel) {

	int32_t miNivel = nivel->nivel;
	int32_t miFd = nivel->fd;
	printf("hola, soy el planificador del nivel %d , mi fd es %d \n", miNivel,
			miFd);
	//inicialización
	//config = config_create(PATH_CONFIG_PLA);
	//char *PATH_LOG = config_get_string_value(config, "PATH_LOG_PLA");
	//logger = log_create(PATH_LOG, "PLANIFICADOR", true, LOG_LEVEL_INFO);
	//inicialización

	char *str_nivel = string_from_format("%d", miNivel);
	t_list *p_listos = dictionary_get(listos, str_nivel);
	enum tipo_paquete tipoMensaje;
	char* mensaje = NULL;
//	t_list *p_monitoreo = dictionary_get(monitoreo, str_nivel);
	while (1) {
		while (list_is_empty(p_listos)) {
			;
		}
		t_pers_por_nivel *personaje = planificar(str_nivel);
		enviarMensaje(personaje->fd, PLA_turnoConcedido_PER, "0");

		if (recibirMensaje(personaje->fd, &tipoMensaje,
				&mensaje)!= EXIT_SUCCESS) {

			//todo: borrarlo de bloqueados , listos, etc y que libere los recursos!
			//eliminarlo de las estructuras

			close(personaje->fd); // ¡Hasta luego!
		} else {
			// tenemos datos del cliente del socket i!
			printf("Llego el tipo de paquete: %s .\n",
					obtenerNombreEnum(tipoMensaje));
			printf("Llego este mensaje: %s .\n", mensaje);

			if (tipoMensaje == PER_posCajaRecurso_PLA) { //no consume quantum
				//pasamanos al nivel, sin procesar nada
				enviarMensaje(nivel->fd, PLA_posCaja_NIV, mensaje);
				free(mensaje);
				recibirMensaje(nivel->fd, &tipoMensaje, &mensaje);
				//if (t_mensaje = NIV_posCaja_PLA){
				enviarMensaje(personaje->fd, PLA_posCajaRecurso_PER, mensaje);
			}
			planificador_analizar_mensaje(personaje, tipoMensaje, mensaje,
					nivel);
			//list_add(p_listos, personaje);
		}

	}
	pthread_exit(NULL );

}

t_pers_por_nivel *planificar(char * str_nivel) {
	//todo : esto es solo para poder hacer las pruebas. hay que desarrollar los algoritmos
	t_list *p_listos = dictionary_get(listos, str_nivel);
	t_pers_por_nivel *aux = list_remove(p_listos, 0); //el primer elemento de la lista

	printf("ahora le toca a %c moverse", aux->personaje);

//	list_add(p_listos, aux);

	return aux;
}
void tratamiento_muerte(int32_t socket, int32_t nivel_fd, char* mensaje,
		char* str_nivel) {

	printf("le aviso al nivel que el personaje %c murio \n", mensaje[0]);
	printf("yo planificador debería liberar recursos? creo que no \n");
	enviarMensaje(nivel_fd, PLA_personajeMuerto_NIV, mensaje);

	t_list *p_listos = dictionary_get(listos, str_nivel);

	int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
		return nuevo->fd == socket;
	}
	t_pers_por_nivel *aux = list_remove_by_condition(p_listos,
			(void*) _esta_personaje); //el primer elemento de la lista

	printf("el personaje %c murio, se lo saca de este nivel %s", aux->personaje,
			str_nivel);
	free(aux);

}

void planificador_analizar_mensaje(t_pers_por_nivel *personaje,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel) {
	char *str_nivel = string_from_format("%d", nivel->nivel);
	enum tipo_paquete t_mensaje;
	char* m_mensaje = NULL;
	switch (tipoMensaje) {

	case PER_movimiento_PLA: {
		//pasamanos al nivel, sin procesar nada
		enviarMensaje(nivel->fd, PLA_movimiento_NIV, mensaje);
		recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);
		//if (t_mensaje = NIV_movimiento_PLA){
		enviarMensaje(personaje->fd, PLA_movimiento_PER, m_mensaje);
		//}
		free(m_mensaje);
		t_list *p_listos = dictionary_get(listos, str_nivel);
		list_add(p_listos, personaje);
		break;
	}
	case PER_recurso_PLA: {
		//pasamanos al nivel, lo paso de listos a bloqueados
		t_list *p_listos = dictionary_get(listos, str_nivel);
		t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);

/*		int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
			return nuevo->fd == personaje->fd;
		}
		t_pers_por_nivel *aux = list_remove_by_condition(p_listos,
				(void*) _esta_personaje); //el primer elemento de la lista
*/
		printf("se bloquea a %c por pedir un recurso", personaje->personaje);
		list_add(p_bloqueados, personaje);

		enviarMensaje(nivel->fd, PLA_solicitudRecurso_NIV, mensaje);
		recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);
		if (t_mensaje == NIV_recursoConcedido_PLA) {
			if (atoi(m_mensaje) == 0) { //recurso concedido

				//
				t_pers_por_nivel *aux = list_remove_by_condition(p_bloqueados,
						(void*) _esta_personaje); //el primer elemento de la lista

				printf("se desbloquea a %c por haber obtenido su recurso",
						aux->personaje);
				list_add(p_listos, aux);
				//

				enviarMensaje(personaje->fd, PLA_rtaRecurso_PER, m_mensaje);

			} else
				printf(
						"el personaje %c sigue bloqueado porque no le dieron el recurso",
						personaje->personaje);
		}
		free(m_mensaje);
		break;
	}
/*	case PER_nivelFinalizado_PLA: {
		//pasamanos al nivel, lo saco de listos
		t_list *p_listos = dictionary_get(listos, str_nivel);
		int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
			return nuevo->fd == personaje->fd;
		}
		t_pers_por_nivel *aux = list_remove_by_condition(p_listos,
				(void*) _esta_personaje); //el primer elemento de la lista
		printf("el personaje %c termino este nivel %d", aux->personaje,
				nivel->nivel);
		free(aux);

		enviarMensaje(nivel->fd, PLA_nivelFinalizado_NIV, mensaje);
		recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);
		//if (t_mensaje = OK){
		enviarMensaje(personaje->fd, OK, m_mensaje);
		//}
		free(m_mensaje);
		break;
	}
	case PER_meMori_PLA: {
		tratamiento_muerte(personaje->fd, nivel->fd, mensaje, str_nivel); //todo
		break;
	}
	*/	default:
		printf("mensaje erroneo");
		break;
	}

	free(mensaje);
}
