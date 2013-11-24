/*
 * planificador.c
 *
 * Created on: 28/09/2013
 * Author: utnso
 */

#include "planificador.h"

#define PATH_LOG_PLA "../planificador.log"

extern t_dictionary *bloqueados;
extern t_dictionary *listos;
extern t_dictionary *anormales;
extern t_dictionary *monitoreo;

extern pthread_mutex_t mutex_listos;
extern pthread_mutex_t mutex_bloqueados;
extern pthread_mutex_t mutex_monitoreo;

t_config *config;
t_log* logger_pla;

void planificador_analizar_mensaje(int32_t socket,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel);
void analizar_mensaje_rta(t_pers_por_nivel *personaje,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel,
		int32_t *quantum);
t_pers_por_nivel *planificar(t_niveles_sistema * str_nivel);
void recurso_concedido(t_pers_por_nivel * personaje, char recurso,
		char *str_nivel);
int tratamiento_recurso(t_pers_por_nivel * personaje, char* str_nivel,
		t_niveles_sistema *nivel, int32_t *quantum);
void tratamiento_asesinato(int32_t nivel_fd, t_pers_por_nivel* personaje,
		char* mensaje, char* str_nivel);

int32_t sumar_valores(char *mensaje) {

	char** n_mensaje = string_split(mensaje, ",");
	int32_t valor = atoi(n_mensaje[0]) + atoi(n_mensaje[1]);

	return valor;
}

void *hilo_planificador(t_niveles_sistema *nivel) {
//log_info(logger_pla,
	int32_t miNivel = nivel->nivel;
	int32_t miFd = nivel->fd;
	char *str_nivel = string_from_format("%d", miNivel);
	//char* cadena = "PLANIFICADOR_Nivel_";
	//char * nueva_cadena = calloc(strlen("PLANIFICADOR_Nivel: "), sizeof(char));
	//strcpy(nueva_cadena, cadena);
	//string_append(&nueva_cadena, str_nivel);
	//inicialización
	//logger_pla = log_create(PATH_LOG_PLA, nueva_cadena, true, LOG_LEVEL_INFO);

	logger_pla = log_create(PATH_LOG_PLA, "PLANIFICADOR", true, LOG_LEVEL_INFO);

	//inicialización
	log_info(logger_pla,
			"Hola, soy el planificador del nivel %d , mi fd es %d ", miNivel,
			miFd);
	log_info(logger_pla,
			"Nivel %d, comienzo con tipo de planificación %s, quantum %d, retardo %d y distancia default para SRTF: %d",
			miNivel, nivel->algol, nivel->quantum, nivel->retardo,
			nivel->remain_distance);

	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	int fdmax; // número máximo de descriptores de fichero
	int i;
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);
	struct timeval tv;
	//el select espera 10 segundos // ver si no es mucho 10 seg
	//tv.tv_sec = 10; //nivel->retardo/1000; //segundos
	tv.tv_usec = nivel->retardo * 1000; //microsegundos
	//voy metiendo aca los personajes para monitorear

	t_list *p_monitoreo = dictionary_get(monitoreo, str_nivel);
	t_list *p_listos = dictionary_get(listos, str_nivel);

	pthread_mutex_lock(&mutex_monitoreo);
	t_monitoreo *aux = list_remove(p_monitoreo, 0); //el primer elemento de la lista
	pthread_mutex_unlock(&mutex_monitoreo);
	FD_SET(aux->fd, &master);
	fdmax = aux->fd;
	free(aux);
	//voy metiendo aca los personajes para monitorear

	int32_t fd_personaje_actual = 0;
	t_pers_por_nivel *personaje = NULL;
	int32_t quantum = nivel->quantum;

	// bucle principal

	for (;;) {
		//planificar a un personaje
		if (!list_is_empty(p_listos) && (fd_personaje_actual == 0)) {
			//hacer el if en base a los algoritmos para calcular el quantum e ir actualizandolo
			personaje = planificar(nivel);
			bool resultado = plan_enviarMensaje(str_nivel, personaje->fd,
					PLA_turnoConcedido_PER, "0");
			if (resultado)
				fd_personaje_actual = personaje->fd;
		}/* else
		 log_info(logger_pla,
		 "Nivel %d: No hay personajes para planificar en este nivel",
		 miNivel);*/
		//planificar a un personaje

		read_fds = master;
		if (select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1) {
			perror("select");
			exit(1);
		}

		//voy metiendo aca los personajes para monitorear
		//int j = 0;
		while (!list_is_empty(p_monitoreo)) {
			pthread_mutex_lock(&mutex_monitoreo);
			aux = list_remove(p_monitoreo, 0);
			pthread_mutex_unlock(&mutex_monitoreo);
			//j++;
			FD_SET(aux->fd, &master); // añadir al conjunto maestro
			if (aux->fd > fdmax) { // actualizar el máximo
				fdmax = aux->fd;
			}
			if (aux->es_personaje) {
				char valor[1];
				valor[0] = aux->simbolo;
				enviarMensaje(nivel->fd, PLA_nuevoPersonaje_NIV, valor);
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
				if (recibirMensaje(i, &tipoMensaje, &mensaje) != EXIT_SUCCESS) {

					t_list *p_muertos = dictionary_get(anormales, str_nivel);
					int32_t * valor = malloc(sizeof(int));
					*valor = i;
					list_add(p_muertos, valor);
					//todo: que libere los recursos y haga gestion de los bloqueados!
					//si era el que estaba planificando liberar los recursos
					//y salir del for asi planifica a alguien mas

					//eliminarlo de las estructuras
					if (i == fd_personaje_actual) {
						//si lo estaba planificando tengo que liberar los recursos que tenía igual y asignarlos.

						proceso_desbloqueo(personaje->recursos_obtenidos,
								nivel->fd, str_nivel);
						destruir_personaje(personaje);

					}
					supr_pers_de_estructuras(i);
					//eliminarlo de las estructuras
					//eliminarlo de las estructuras

					close(i); // ¡Hasta luego!
					FD_CLR(i, &master); // eliminar del conjunto maestro
					break;

					if (i == fd_personaje_actual)
						break;

				} else {
					// tenemos datos del cliente del socket i!
					log_info(logger_pla,
							"Nivel %s: Llego el tipo de paquete: %s .",
							str_nivel, obtenerNombreEnum(tipoMensaje));
					log_info(logger_pla, "Nivel %s: Llego este mensaje: %s .",
							str_nivel, mensaje);
					log_info(logger_pla,
							"Nivel %s: se esperaba la respuesta de %d .",
							str_nivel, fd_personaje_actual);
					log_info(logger_pla,
							"Nivel %s: se recibió el mensaje de %d .",
							str_nivel, i);

					if (i == fd_personaje_actual) {
						if (tipoMensaje == PER_posCajaRecurso_PLA) { //no consume quantum
							//pasamanos al nivel, sin procesar nada
							log_info(logger_pla,
									"Nivel %s: Envio al nivel solicitud posicion caja %s",
									str_nivel, mensaje);
							enviarMensaje(nivel->fd, PLA_posCaja_NIV, mensaje);
							free(mensaje);
							recibirMensaje(nivel->fd, &tipoMensaje, &mensaje);
							log_info(logger_pla,
									"Llego el tipo de paquete: %s .",
									obtenerNombreEnum(tipoMensaje));
							log_info(logger_pla, "Llego este mensaje: %s .",
									mensaje);
							if (tipoMensaje == NIV_posCaja_PLA) {

								log_info(logger_pla,
										"Nivel %s: envio al personaje %c posicion de la caja %s",
										str_nivel, personaje->personaje,
										mensaje);
								bool resultado = plan_enviarMensaje(str_nivel,
										i, PLA_posCajaRecurso_PER, mensaje);
								if (resultado)
									personaje->pos_recurso = sumar_valores(
											mensaje);
								else {
									proceso_desbloqueo(
											personaje->recursos_obtenidos,
											nivel->fd, str_nivel);
									destruir_personaje(personaje);
									supr_pers_de_estructuras(i);
								}

							} else {
								if (tipoMensaje == NIV_enemigosAsesinaron_PLA) {
									log_info(logger_pla,
											"Nivel %s: Recibo el asesinato de: %s",
											str_nivel, mensaje);

									tratamiento_asesinato(nivel->fd, personaje,
											mensaje, str_nivel);
								} else {
									t_list *p_muertos = dictionary_get(
											anormales, str_nivel);
									int32_t * valor = malloc(sizeof(int));
									*valor = i;
									list_add(p_muertos, valor);
									proceso_desbloqueo(
											personaje->recursos_obtenidos,
											nivel->fd, str_nivel);
									destruir_personaje(personaje);
									supr_pers_de_estructuras(i);
								}
							}

							recibirMensaje(i, &tipoMensaje, &mensaje);

							log_info(logger_pla,
									"Nivel %s: Llego el tipo de paquete: %s .",
									str_nivel, obtenerNombreEnum(tipoMensaje));
							log_info(logger_pla,
									"Nivel %s: Llego este mensaje: %s .",
									str_nivel, mensaje);
						}

						analizar_mensaje_rta(personaje, tipoMensaje, mensaje,
								nivel, &quantum);
						fd_personaje_actual = 0;
					} else {

						if (tipoMensaje == NIV_enemigosAsesinaron_PLA) {
							log_info(logger_pla,
									"Nivel %s: Recibo el asesinato de: %s",
									str_nivel, mensaje);

							tratamiento_asesinato(nivel->fd, personaje, mensaje,
									str_nivel);
						} else

							planificador_analizar_mensaje(i, tipoMensaje,
									mensaje, nivel);
//                                                fd_personaje_actual = 0;
//                                                destruir_personaje(personaje);
					}
				} // fin seccion recibir OK los datos
			} // fin de tenemos datos
		}
	} // fin bucle for principal

	pthread_exit(NULL );

}

t_pers_por_nivel *planificar(t_niveles_sistema* nivel) {
	char *str_nivel = string_from_format("%d", nivel->nivel);
	t_list *p_listos = dictionary_get(listos, str_nivel);
	t_pers_por_nivel *aux;

	if (!string_equals_ignore_case(nivel->algol, "RR")) {

		//encontrar el de menor distancia al recurso
		bool _menor_distancia(t_pers_por_nivel *cerca,
				t_pers_por_nivel *menos_cerca) {
			int primero = abs(cerca->pos_recurso - cerca->pos_inicial);
			int segundo = abs(
					menos_cerca->pos_recurso - menos_cerca->pos_inicial);

			return primero <= segundo;
		}

		list_sort(p_listos, (void*) _menor_distancia);
	}

	pthread_mutex_lock(&mutex_listos);
	aux = list_remove(p_listos, 0); //el primer elemento de la lista
	pthread_mutex_unlock(&mutex_listos);

	log_info(logger_pla, "Nivel %d: Sale %c de la lista de listos",
			nivel->nivel, aux->personaje);

	imprimir_lista(LISTA_LISTOS, string_from_format("%d", nivel->nivel));

	log_info(logger_pla, "Nivel %d: Ahora le toca a %c moverse", nivel->nivel,
			aux->personaje);
	return aux;
}

void tratamiento_muerte(int32_t socket_l, int32_t nivel_fd, char* mensaje,
		char* str_nivel) {
	t_list *p_muertos = dictionary_get(anormales, str_nivel);
	int32_t * valor = malloc(sizeof(int));
	*valor = socket_l;
	list_add(p_muertos, valor);
	log_info(logger_pla, "le aviso al nivel que el personaje %c murio ",
			mensaje[0]);
	// log_info(logger_pla, "yo planificador debería liberar recursos? SI ");
	enviarMensaje(nivel_fd, PLA_personajeMuerto_NIV, mensaje);

	int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
		return nuevo->fd == socket_l;
	}
	pthread_mutex_lock(&mutex_listos);
	//t_pers_por_nivel *aux = list_remove_by_condition(p_listos, //matyx
	//		(void*) _esta_personaje); //el primer elemento de la lista
	pthread_mutex_unlock(&mutex_listos);
	log_info(logger_pla,
			"Nivel %d: El personaje del socket %d murio, se lo saca de este nivel",
			str_nivel, (int) valor);
	supr_pers_de_estructuras(socket_l);
	//free(aux);

}
int tratamiento_recurso(t_pers_por_nivel * personaje, char* str_nivel,
		t_niveles_sistema *nivel, int32_t *quantum);
void analizar_mensaje_rta(t_pers_por_nivel *personaje,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel,
		int32_t *quantum) {
	char *str_nivel = string_from_format("%d", nivel->nivel);
	enum tipo_paquete t_mensaje;
	char* m_mensaje = NULL;

	//t_list *p_listos = dictionary_get(listos, str_nivel);
	//t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);

	int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
		return nuevo->fd == personaje->fd;
	}

	switch (tipoMensaje) {

	case PER_movimiento_PLA: {
		//pasamanos al nivel, sin procesar nada

		personaje->pos_inicial = sumar_valores(mensaje);

		char *prueba = calloc(3, sizeof(char));
		prueba[0] = personaje->personaje;
		prueba[1] = ',';
		string_append(&prueba, mensaje);
		//log_info(logger_pla, "el mensaje que mando es: %s ", prueba);
		//log_info(logger_pla, "la longitud del mensaje que mando es: %d ",
		//               strlen(prueba));
		log_info(logger_pla,
				"Nivel %d: Mando al nivel la solicitud de movimiento: %s ",
				nivel->nivel, prueba);
		enviarMensaje(nivel->fd, PLA_movimiento_NIV, prueba);
		free(prueba);
		recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);
		if (t_mensaje == NIV_movimiento_PLA) {
			log_info(logger_pla,
					"Nivel %d: Mando al personaje %c la respuesta al movimiento: %s ",
					nivel->nivel, personaje->personaje, m_mensaje);
			bool resultado = plan_enviarMensaje(str_nivel, personaje->fd,
					PLA_movimiento_PER, m_mensaje);

			if (resultado) {
				int res = tratamiento_recurso(personaje, str_nivel, nivel,
						quantum);

				if (res == 1) {
					free(m_mensaje);
					break;
				}
			} else {
				proceso_desbloqueo(personaje->recursos_obtenidos, nivel->fd,
						str_nivel);
				supr_pers_de_estructuras(personaje->fd);
				destruir_personaje(personaje);
			}
		} else {
			//TODO Danii, aca me parece que deberia poder recibir el mensaje de que el personaje murio por enemigo, es la unica forma que se me ocurre en caso de que el personaje se mueva y el enemigo lo agarre justo, si no puedo recibir aca el mensaje de que el enemigo lo mató, entonces nunca se le va a responder que el turno estaba bien o mal, porque va a recibir el mensaje del personaje muerto por enemigo
			if (t_mensaje == NIV_enemigosAsesinaron_PLA) {
				log_info(logger_pla, "Nivel %d: Recibo el asesinato de: %s",
						nivel->nivel, m_mensaje);

				tratamiento_asesinato(nivel->fd, personaje, m_mensaje,
						str_nivel);
				recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);//matyx

				recibirMensaje(personaje->fd, &t_mensaje, &m_mensaje);//matyx

				char * simbolo = string_new();

				string_append(&simbolo,string_from_format("%c", personaje->personaje));
				tratamiento_muerte(personaje->fd, nivel->fd, simbolo, str_nivel);
				enviarMensaje(personaje->fd, OK1,"0");
			} else {
				t_list *p_muertos = dictionary_get(anormales, str_nivel);
				int32_t * valor = malloc(sizeof(int));
				*valor = personaje->fd;
				list_add(p_muertos, valor);
				proceso_desbloqueo(personaje->recursos_obtenidos, nivel->fd,
						str_nivel);

				supr_pers_de_estructuras(personaje->fd);
				destruir_personaje(personaje);

//				supr_pers_de_estructuras(personaje->fd);
				break;
			}
		}

		free(m_mensaje);

		break;
	}
	case PER_nivelFinalizado_PLA: {
		//pasamanos al nivel, lo saco de listos
		/*t_list *p_listos = dictionary_get(listos, str_nivel);
		 int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
		 return nuevo->fd == personaje->fd;
		 }
		 pthread_mutex_lock(&mutex_listos);
		 t_pers_por_nivel *aux = list_remove_by_condition(p_listos,
		 (void*) _esta_personaje); //el primer elemento de la lista
		 pthread_mutex_unlock(&mutex_listos);*/
		log_info(logger_pla, "Nivel %d: El personaje %c termino este nivel",
				nivel->nivel, personaje->personaje);
		proceso_desbloqueo(personaje->recursos_obtenidos, nivel->fd, str_nivel);
		destruir_personaje(personaje);
		log_info(logger_pla,
				"Nivel %d: Mando al nivel que el personaje %c terminó este nivel ",
				nivel->nivel, personaje->personaje);
		enviarMensaje(nivel->fd, PLA_nivelFinalizado_NIV, mensaje);
		/*                enum tipo_paquete t_mensaje;
		 char* m_mensaje = NULL;
		 recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);
		 */break;
	}
	default: {
		log_info(logger_pla, "Nivel %d: El personaje %c se desconecto ",
				nivel->nivel, personaje->personaje);
		t_list *p_muertos = dictionary_get(anormales, str_nivel);
		int32_t * valor = malloc(sizeof(int));
		*valor = personaje->fd;
		list_add(p_muertos, valor);
		proceso_desbloqueo(personaje->recursos_obtenidos, nivel->fd, str_nivel);
		destruir_personaje(personaje);
		supr_pers_de_estructuras(personaje->fd);
		break;
	}
	}
	free(mensaje);

}
void recurso_concedido(t_pers_por_nivel * personaje, char recurso,
		char *str_nivel);
int tratamiento_recurso(t_pers_por_nivel * personaje, char* str_nivel,
		t_niveles_sistema *nivel, int32_t *quantum) {
	int res = 0;
	//pasamanos al nivel, lo paso de listos a bloqueados

	enum tipo_paquete k_mensaje;
	char* j_mensaje = NULL;

	t_list *p_listos = dictionary_get(listos, str_nivel);
	t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);

	recibirMensaje(personaje->fd, &k_mensaje, &j_mensaje);

	log_info(logger_pla, "Llego el tipo de paquete: %s .",
			obtenerNombreEnum(k_mensaje));
	log_info(logger_pla, "Llego este mensaje: %s .", j_mensaje);

	if (k_mensaje == PER_recurso_PLA) {

		if (!string_equals_ignore_case(j_mensaje, "0")) {

			personaje->pos_inicial = personaje->pos_recurso
					+ nivel->remain_distance;

			log_info(logger_pla,
					"Nivel %s: Se bloquea a %c por pedir un recurso", str_nivel,
					personaje->personaje);
			pthread_mutex_lock(&mutex_bloqueados);
			list_add(p_bloqueados, personaje);
			pthread_mutex_unlock(&mutex_bloqueados);
			imprimir_lista(LISTA_BLOQUEADOS, str_nivel);
			char recurso = j_mensaje[0];
			int32_t _esta_recurso(t_recursos_obtenidos *nuevo) {
				return nuevo->recurso == recurso;
			}
			char * pers = string_from_format("%c", personaje->personaje);
			string_append(&pers, ",");
			string_append(&pers, j_mensaje);
			log_info(logger_pla,
					"Nivel %d: Mando al nivel la solicitud del recurso %s ",
					nivel->nivel, pers);
			enviarMensaje(nivel->fd, PLA_solicitudRecurso_NIV, pers);
			free(j_mensaje);
			recibirMensaje(nivel->fd, &k_mensaje, &j_mensaje);
			if (k_mensaje == NIV_recursoConcedido_PLA) {
				if (atoi(j_mensaje) == 0) { //recurso concedido
					res = 0;
					recurso_concedido(personaje, recurso, str_nivel);
					log_info(logger_pla,
							"Nivel %d: Al personaje %c le dimos el recurso %c",
							nivel->nivel, personaje->personaje, recurso);
				} else {
					res = 1;
					log_info(logger_pla,
							"Nivel %d: El personaje %c sigue bloqueado porque no le dieron el recurso",
							nivel->nivel, personaje->personaje);
					personaje->recurso_bloqueo = recurso;
					personaje->estoy_bloqueado = true;

				}

			} else {
				proceso_desbloqueo(personaje->recursos_obtenidos, nivel->fd,
						str_nivel);

				supr_pers_de_estructuras(personaje->fd);
				destruir_personaje(personaje);

			}
			if (string_equals_ignore_case(nivel->algol, "RR")) {
				(*quantum) = nivel->quantum;
				log_info(logger_pla, "Nivel %d: Valor del quantum %d",
						nivel->nivel, *quantum);
			}
			free(j_mensaje);
		} else {

			//modificar que si pidieron un recurso no se entre aca
			if (string_equals_ignore_case(nivel->algol, "RR")) {
				(*quantum)--;
				if ((*quantum) != 0) {
					pthread_mutex_lock(&mutex_listos);
					//lo pone primero asi despues sigue el mismo planificandose
					list_add_in_index(p_listos, 0, personaje);
					pthread_mutex_unlock(&mutex_listos);
					log_info(logger_pla,
							"Nivel %d: Se agrega al personaje %c a la lista de listos",
							nivel->nivel, personaje->personaje);
					imprimir_lista(LISTA_LISTOS, str_nivel);
				} else {
					//lo pone al final
					pthread_mutex_lock(&mutex_listos);
					list_add(p_listos, personaje);
					pthread_mutex_unlock(&mutex_listos);
					(*quantum) = nivel->quantum;
					log_info(logger_pla,
							"Nivel %d: Se agrega al personaje %c a la lista de listos",
							nivel->nivel, personaje->personaje);
					imprimir_lista(LISTA_LISTOS, str_nivel);

				}
				log_info(logger_pla, "Nivel %d: Valor del quantum %d",
						nivel->nivel, *quantum);
			} else {
				pthread_mutex_lock(&mutex_listos);
				list_add(p_listos, personaje);
				pthread_mutex_unlock(&mutex_listos);
				log_info(logger_pla,
						"Nivel %d: Se agrega al personaje %c a la lista de listos",
						nivel->nivel, personaje->personaje);
				imprimir_lista(LISTA_LISTOS, str_nivel);

			}

		}
	} else {
		proceso_desbloqueo(personaje->recursos_obtenidos, nivel->fd, str_nivel);

		supr_pers_de_estructuras(personaje->fd);
		destruir_personaje(personaje);
	}
	return res;
}
void recurso_concedido(t_pers_por_nivel * personaje, char recurso,
		char *str_nivel) {
	t_list *p_listos = dictionary_get(listos, str_nivel);
	t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);

	int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
		return nuevo->fd == personaje->fd;
	}
	int32_t _esta_recurso(t_recursos_obtenidos *nuevo) {
		return nuevo->recurso == recurso;
	}
	//
	pthread_mutex_lock(&mutex_bloqueados);
	t_pers_por_nivel *aux = list_remove_by_condition(p_bloqueados,
			(void*) _esta_personaje);
	pthread_mutex_unlock(&mutex_bloqueados);
	log_info(logger_pla,
			"Nivel %s: Se desbloquea a %c por haber obtenido su recurso",
			str_nivel, aux->personaje);
	imprimir_lista(LISTA_BLOQUEADOS, str_nivel);

	pthread_mutex_lock(&mutex_listos);
	if (list_is_empty(aux->recursos_obtenidos)) {

		t_recursos_obtenidos *rec = malloc(sizeof(t_recursos_obtenidos));
		rec->recurso = recurso;
		rec->cantidad = 1;
		list_add(aux->recursos_obtenidos, rec);
	} else {
		t_recursos_obtenidos *rec = list_find(aux->recursos_obtenidos,
				(void*) _esta_recurso);
		if (rec == NULL ) {

			rec = malloc(sizeof(t_recursos_obtenidos));
			rec->recurso = recurso;
			rec->cantidad = 1;
			list_add(aux->recursos_obtenidos, rec);
		} else
			rec->cantidad++;
	}
	list_add(p_listos, aux);
	pthread_mutex_unlock(&mutex_listos);
	log_info(logger_pla,
			"Nivel %s: Se agrega al personaje %c a la lista de listos",
			str_nivel, aux->personaje);
	imprimir_lista(LISTA_LISTOS, str_nivel);

	//
	//log_info(logger_pla, "Nivel %d: Se responde al personaje %c que obtu",nivel->nivel, personaje->personaje);

	bool resultado = plan_enviarMensaje(str_nivel, aux->fd, PLA_rtaRecurso_PER,
			"0");
	if (!resultado) {
		proceso_desbloqueo(personaje->recursos_obtenidos, atoi(str_nivel),
				str_nivel);
		destruir_personaje(personaje);
		supr_pers_de_estructuras(personaje->fd);
	}

}

char * transformarListaCadena(t_list *recursosDisponibles) {
	char *recursosNuevos = string_new(); //"F,1;T,6;M,12";
	char *recu, *cant;
	int i = 0;
	while (!list_is_empty(recursosDisponibles)) {
		t_recursos_obtenidos * valor = list_remove(recursosDisponibles, 0);
		recu = string_from_format("%c", valor->recurso);
		if (i != 0)
			string_append(&recursosNuevos, ";");
		string_append(&recursosNuevos, recu);
		//log_info(logger_pla, "cadena en proceso %s", recursosNuevos);
		cant = string_from_format("%d", valor->cantidad);
		string_append(&recursosNuevos, ",");
		string_append(&recursosNuevos, cant);
		//log_info(logger_pla, "cadena en proceso %s", recursosNuevos);
		i++;
	}
	char *cantidad = string_from_format("%d", i);
	string_append(&cantidad, ";");

	string_append(&cantidad, recursosNuevos);

	//log_info(logger_pla, "cadena final %s", cantidad);
	return cantidad;
}

char * transformarListaCadena_interbloq(t_list *personajesDisponibles) {
	char *personajesInterb = string_new(); //"F,1,T,6,M,12";

	/*
	 *
	 * Ejemplo "@,H,F;%,M,H,F;#,F,M"
	 eso significa que: @ tiene H y está bloqueado por F
	 % tiene M y H y está bloqueado por F
	 # tiene F y está bloqueado por M
	 * */
	char *recu, *pers;
	int i;
	while (!list_is_empty(personajesDisponibles)) {
		t_pers_por_nivel * valor = list_remove(personajesDisponibles, i);
		pers = string_from_format("%c", valor->personaje);
		if (i != 0)
			string_append(&personajesInterb, ";");
		string_append(&personajesInterb, pers);
		string_append(&personajesInterb, ",");
		//log_info(logger_pla, "cadena en proceso %s", personajesInterb);

		int m = 0;
		t_recursos_obtenidos *recursos = list_get(valor->recursos_obtenidos, m);
		while (recursos != NULL ) {
			m++;
			recu = string_from_format("%d", recursos->recurso);
			string_append(&personajesInterb, recu);
			string_append(&personajesInterb, ",");

			recursos = list_get(valor->recursos_obtenidos, m);

		}
		recu = string_from_format("%c", valor->recurso_bloqueo);
		string_append(&personajesInterb, recu);
		//log_info(logger_pla, "cadena en proceso %s", personajesInterb);
		i++;
	}
	//log_info(logger_pla, "cadena final %s", personajesInterb);
	return personajesInterb;
}

void proceso_desbloqueo(t_list *recursos, int32_t fd, char *str_nivel) {
	t_list *recursosDisponibles = desbloquear_personajes(recursos, str_nivel,
			fd);
	char *recursosNuevos = transformarListaCadena(recursosDisponibles);

	if (!string_equals_ignore_case(recursosNuevos, "0;")) {
		log_info(logger_pla,
				"Nivel %s: Le envio al nivel los recursos disponibles %s",
				str_nivel, recursosNuevos);
		enviarMensaje(fd, PLA_actualizarRecursos_NIV, recursosNuevos);
	}
	int m;
	t_recursos_obtenidos *elem;
	for (m = 0; !list_is_empty(recursosDisponibles); m++) {
		elem = list_remove(recursosDisponibles, m);
		free(elem);
	}

	list_destroy(recursosDisponibles);
}

void tratamiento_asesinato(int32_t nivel_fd, t_pers_por_nivel* personaje,
		char* mensaje, char* str_nivel) {

	t_list *p_muertos = dictionary_get(anormales, str_nivel);
	int32_t * valor = malloc(sizeof(int));
	*valor = personaje->fd;
	list_add(p_muertos, valor);
	// log_info(logger_pla, "%s" Dani aca me devuelve una lista vacia y rompe, pero recibió el mensaje de que un enemigo asesino un personaje");
	t_pers_por_nivel *aux = personaje;

	int j, i = 0;
	t_list *recursos = list_create();
	while (mensaje[i] != '\0') {

		if (aux == NULL ) {
			int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
				return nuevo->personaje == mensaje[i];
			}

			pthread_mutex_lock(&mutex_listos);
			//personaje = list_remove_by_condition(p_listos, // matyx
	//							(void*) _esta_personaje);
			pthread_mutex_unlock(&mutex_listos);

			log_info(logger_pla,
					"Nivel %s: Se saca a %c de la lista de listos por haber muerto interbloqueado",
					str_nivel, aux->personaje);
			imprimir_lista(LISTA_BLOQUEADOS, str_nivel);
		}

		j = 0;
		while (!list_is_empty(aux->recursos_obtenidos)) {
			t_recursos_obtenidos* rec = list_remove(aux->recursos_obtenidos, j);

			int32_t _esta_recurso(t_recursos_obtenidos *nuevo) {
				return nuevo->recurso == rec->recurso;
			}
			//busco si ese mismo recurso ya lo habia liberado alguno
			t_recursos_obtenidos* recu = list_find(recursos,
					(void*) _esta_recurso);
			if (recu == NULL )
				list_add(recursos, rec);
			else {
				recu->cantidad += rec->cantidad;
				free(rec);
			}

			//j++;
		}
		log_info(logger_pla,
				"Nivel %s: Aviso al personaje %c que los enemigos lo asesinaron",
				str_nivel, aux->personaje);
		plan_enviarMensaje(str_nivel, aux->fd,
				PLA_teMatamos_PER, "0");

		//destruir_personaje(personaje);// matyx
		i++;
	}

	proceso_desbloqueo(recursos, nivel_fd, str_nivel);

}

void planificador_analizar_mensaje(int32_t socket_r,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel) {
	char *str_nivel = string_from_format("%d", nivel->nivel);

	switch (tipoMensaje) {

	case PER_nivelFinalizado_PLA: {
		//pasamanos al nivel, lo saco de listos
		t_list *p_muertos = dictionary_get(anormales, str_nivel);
		int32_t * valor = malloc(sizeof(int));
		*valor = socket_r;
		list_add(p_muertos, valor);
		t_list *p_listos = dictionary_get(listos, str_nivel);
		int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
			return nuevo->fd == socket_r;
		}
		pthread_mutex_lock(&mutex_listos);
		t_pers_por_nivel *aux = list_remove_by_condition(p_listos,
				(void*) _esta_personaje); //el primer elemento de la lista
		pthread_mutex_unlock(&mutex_listos);
		log_info(logger_pla, "Nivel %s: El personaje %c termino este nivel",
				str_nivel, aux->personaje);
		proceso_desbloqueo(aux->recursos_obtenidos, nivel->fd, str_nivel);
		destruir_personaje(aux);

		log_info(logger_pla,
				"Nivel %d: Informo al nivel que el personaje %c termino",
				nivel->nivel, mensaje);
		enviarMensaje(nivel->fd, PLA_nivelFinalizado_NIV, mensaje);
		/*                enum tipo_paquete t_mensaje;
		 char* m_mensaje = NULL;
		 recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);
		 */break;
	}
	case PER_meMori_PLA: {
		tratamiento_muerte(socket_r, nivel->fd, mensaje, str_nivel);
		break;
	}
	case NIV_cambiosConfiguracion_PLA: {
		//mensaje = algoritmo,quantum,retardo = "RR,4,1000"
		char** n_mensaje = string_split(mensaje, ",");
		nivel->algol = n_mensaje[0];
		nivel->quantum = atoi(n_mensaje[1]);
		nivel->retardo = atoi(n_mensaje[2]);

		break;
	}
	case NIV_enemigosAsesinaron_PLA: {

		log_info(logger_pla, "Nivel %s: Recibo el asesinato de: %s", str_nivel,
				mensaje);
		tratamiento_asesinato(nivel->fd, NULL, mensaje, str_nivel);
		break;
	}
	case NIV_perMuereInterbloqueo_PLA: {

		//le aviso al nivel que el personaje murio
		enviarMensaje(nivel->fd,PLA_perMuereInterbloqueo_NIV,mensaje);

		t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);

		int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
			return nuevo->personaje == mensaje[0];
		}

		pthread_mutex_lock(&mutex_bloqueados);
		t_pers_por_nivel *aux = list_remove_by_condition(p_bloqueados,
				(void*) _esta_personaje);
		pthread_mutex_unlock(&mutex_bloqueados);

		//le aviso al personaje que murio
		enviarMensaje(aux->fd,PLA_rtaRecurso_PER,"1");

		proceso_desbloqueo(aux->recursos_obtenidos, nivel->fd, str_nivel);

		t_list *p_muertos = dictionary_get(anormales, str_nivel);
		int32_t * valor = malloc(sizeof(int));
		*valor = aux->fd;
		list_add(p_muertos, valor);

		destruir_personaje(aux);
		break;
	}
		/*case NIV_recursosPersonajesBloqueados_PLA: {

		 t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);

		 int32_t _podria_interbloquearse(t_pers_por_nivel *nuevo) {
		 return !list_is_empty(nuevo->recursos_obtenidos);
		 }

		 t_list *lista = list_filter(p_bloqueados,
		 (void*) _podria_interbloquearse);

		 char* respuesta = transformarListaCadena_interbloq(lista);
		 enviarMensaje(nivel->fd, PLA_recursosPersonajesBloqueados_NIV,
		 respuesta);

		 break;
		 }
		 */

	default: {
		t_list *p_muertos = dictionary_get(anormales, str_nivel);
		int32_t * valor = malloc(sizeof(int));
		*valor = socket_r;
		list_add(p_muertos, valor);
		log_info(logger_pla, "Nivel %s: Recibí un mensaje erroneo del fd %d",
				str_nivel, socket_r);
		supr_pers_de_estructuras(socket_r);
		break;
	}
	}

	free(mensaje);
}

bool plan_enviarMensaje(char* str_nivel, int32_t fd, enum tipo_paquete paquete,
		char* mensaje) {

	int32_t *fd_muerto = NULL;
	int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
		return nuevo->fd == fd;
	}

	t_list *p_muertos = dictionary_get(anormales, str_nivel);
	fd_muerto = list_find(p_muertos, (void*) _esta_personaje);

	if (fd_muerto == NULL ) {
		if (enviarMensaje(fd, paquete, mensaje) != EXIT_SUCCESS)
			return false;
		else
			return true;
	} else {
		log_info(logger_pla,
				"Nivel %s: Personaje desconectado, no se puede enviar el mensaje",
				str_nivel);
		return false;
	}
}
