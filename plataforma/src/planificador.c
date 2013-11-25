/*
 * planificador.c
 *
 * Created on: 28/09/2013
 * Author: utnso
 */

#include "planificador.h"

#define PATH_LOG_PLA "./planificador.log"

extern t_dictionary *bloqueados;
extern t_dictionary *listos;
extern t_dictionary *anormales;
extern t_dictionary *monitoreo;

extern pthread_mutex_t mutex_listos;
extern pthread_mutex_t mutex_bloqueados;
extern pthread_mutex_t mutex_monitoreo;
extern pthread_mutex_t mutex_anormales;
extern pthread_mutex_t mutex_log;

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

void agregar_anormales(char* str_nivel, int32_t fd) {

	t_list *p_muertos = dictionary_get(anormales, str_nivel);
	int32_t _esta_muerto(t_pers_por_nivel *valor) {
		return valor->fd == fd;
	}

	pthread_mutex_lock(&mutex_anormales);
	t_pers_por_nivel* auxiliar = list_find(p_muertos, (void*) _esta_muerto);
	pthread_mutex_unlock(&mutex_anormales);

	if (auxiliar == NULL ) {
		int32_t * valor = malloc(sizeof(int));
		*valor = fd;
		list_add(p_muertos, valor);
	}

}

void *hilo_planificador(t_niveles_sistema *nivel) {

	int32_t miNivel = nivel->nivel;
	int32_t miFd = nivel->fd;
	char *str_nivel = string_from_format("%d", miNivel);

	logger_pla = log_create(PATH_LOG_PLA, "PLANIFICADOR", true, LOG_LEVEL_INFO);

	//inicialización
	pthread_mutex_lock(&mutex_log); log_info(logger_pla,
			"Hola, soy el planificador del nivel %d , mi fd es %d ", miNivel,
			miFd);
	pthread_mutex_unlock(&mutex_log);
	pthread_mutex_lock(&mutex_log); log_info(logger_pla,
			"Nivel %d, comienzo con tipo de planificación %s, quantum %d, retardo %d y distancia default para SRTF: %d",
			miNivel, nivel->algol, nivel->quantum, nivel->retardo,
			nivel->remain_distance);
	pthread_mutex_unlock(&mutex_log);

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

	int32_t _esta_nivel(t_monitoreo *valor) {
		return valor->es_personaje == false;
	}

	pthread_mutex_lock(&mutex_monitoreo);
	t_monitoreo *aux = list_remove_by_condition(p_monitoreo,
			(void*) _esta_nivel); //el elemento nivel
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
		 pthread_mutex_lock(&mutex_log); log_info(logger_pla,
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

					//eliminarlo de las estructuras
					if (i == fd_personaje_actual) {
						//si lo estaba planificando tengo que liberar los recursos que tenía igual y asignarlos.

						pthread_mutex_lock(&mutex_log); log_info(logger_pla,
								"Nivel %s: EL personaje %c que se estaba planificando ha muerto",
								str_nivel, personaje->personaje);
						pthread_mutex_unlock(&mutex_log);
						pthread_mutex_lock(&mutex_log); log_info(logger_pla,
								"Nivel %s: Se procede a eliminarlo de las estructuras y avisarle al nivel",
								str_nivel);
						pthread_mutex_unlock(&mutex_log);
						suprimir_personaje_de_estructuras(personaje);

					} else {
						pthread_mutex_lock(&mutex_log); log_info(logger_pla,
								"Nivel %s: cliente del socket %d se ha descoenctado",
								str_nivel, i);
						pthread_mutex_unlock(&mutex_log);

						suprimir_de_estructuras(i);
					}
					//eliminarlo de las estructuras

					close(i); // ¡Hasta luego!
					FD_CLR(i, &master); // eliminar del conjunto maestro
					break;

					if (i == fd_personaje_actual)
						break;

				} else {
					// tenemos datos del cliente del socket i!
					pthread_mutex_lock(&mutex_log); log_info(logger_pla,
							"Nivel %s: Llego el tipo de paquete: %s .",
							str_nivel, obtenerNombreEnum(tipoMensaje));
					pthread_mutex_unlock(&mutex_log);
					pthread_mutex_lock(&mutex_log); log_info(logger_pla, "Nivel %s: Llego este mensaje: %s .",
							str_nivel, mensaje);
					pthread_mutex_unlock(&mutex_log);
					pthread_mutex_lock(&mutex_log); log_info(logger_pla,
							"Nivel %s: se esperaba la respuesta de %d .",
							str_nivel, fd_personaje_actual);
					pthread_mutex_unlock(&mutex_log);
					pthread_mutex_lock(&mutex_log); log_info(logger_pla,
							"Nivel %s: se recibió el mensaje de %d .",
							str_nivel, i);
					pthread_mutex_unlock(&mutex_log);

					if (i == fd_personaje_actual) {
						if (tipoMensaje == NIV_cambiosConfiguracion_PLA) {

							//mensaje = algoritmo,quantum,retardo = "RR,4,1000"
							char** n_mensaje = string_split(mensaje, ",");
							nivel->algol = n_mensaje[0];
							nivel->quantum = atoi(n_mensaje[1]);
							nivel->retardo = atoi(n_mensaje[2]);

							recibirMensaje(i, &tipoMensaje, &mensaje);

							pthread_mutex_lock(&mutex_log); log_info(logger_pla,
									"Nivel %s: Llego el tipo de paquete: %s .",
									str_nivel, obtenerNombreEnum(tipoMensaje));
							pthread_mutex_unlock(&mutex_log);
							pthread_mutex_lock(&mutex_log); log_info(logger_pla,
									"Nivel %s: Llego este mensaje: %s .",
									str_nivel, mensaje);
							pthread_mutex_unlock(&mutex_log);

						} else {
							if (tipoMensaje == PER_posCajaRecurso_PLA) { //no consume quantum
								//pasamanos al nivel, sin procesar nada
								pthread_mutex_lock(&mutex_log); log_info(logger_pla,
										"Nivel %s: Envio al nivel solicitud posicion caja %s",
										str_nivel, mensaje);
								pthread_mutex_unlock(&mutex_log);
								enviarMensaje(nivel->fd, PLA_posCaja_NIV,
										mensaje);
								free(mensaje);
								recibirMensaje(nivel->fd, &tipoMensaje,
										&mensaje);
								pthread_mutex_lock(&mutex_log); log_info(logger_pla,
										"Nivel %s: Llego el tipo de paquete: %s .",
										str_nivel,
										obtenerNombreEnum(tipoMensaje));
								pthread_mutex_unlock(&mutex_log);
								pthread_mutex_lock(&mutex_log); log_info(logger_pla,
										"Nivel %s: Llego este mensaje: %s .",
										str_nivel, mensaje);
								pthread_mutex_unlock(&mutex_log);
								if (tipoMensaje == NIV_posCaja_PLA) {

									bool resultado = plan_enviarMensaje(
											str_nivel, i,
											PLA_posCajaRecurso_PER, mensaje);
									if (resultado) {
										personaje->pos_recurso = sumar_valores(
												mensaje);
										pthread_mutex_lock(&mutex_log); log_info(logger_pla,
												"Nivel %s: envio al personaje %c posicion de la caja %s",
												str_nivel, personaje->personaje,
												mensaje);
										pthread_mutex_unlock(&mutex_log);
									} else {
										suprimir_personaje_de_estructuras(
												personaje);
									}

								} else {
									if (tipoMensaje
											== NIV_enemigosAsesinaron_PLA) {
										pthread_mutex_lock(&mutex_log); log_info(logger_pla,
												"Nivel %s: Recibo el asesinato de: %s",
												str_nivel, mensaje);
										pthread_mutex_unlock(&mutex_log);
										tratamiento_asesinato(nivel->fd,
												personaje, mensaje, str_nivel);
									} else {
										if (tipoMensaje
												== NIV_cambiosConfiguracion_PLA) {

											//mensaje = algoritmo,quantum,retardo = "RR,4,1000"
											char** n_mensaje = string_split(
													mensaje, ",");
											nivel->algol = n_mensaje[0];
											nivel->quantum = atoi(n_mensaje[1]);
											nivel->retardo = atoi(n_mensaje[2]);
											enviarMensaje(nivel->fd, OK1, "0");
											recibirMensaje(nivel->fd,
													&tipoMensaje, &mensaje);

											pthread_mutex_lock(&mutex_log); log_info(logger_pla,
													"Nivel %s: Llego el tipo de paquete: %s .",
													str_nivel,
													obtenerNombreEnum(
															tipoMensaje));
											pthread_mutex_unlock(&mutex_log);
											pthread_mutex_lock(&mutex_log); log_info(logger_pla,
													"Nivel %s: Llego este mensaje: %s .",
													str_nivel, mensaje);
											pthread_mutex_unlock(&mutex_log);
											if (tipoMensaje
													== NIV_posCaja_PLA) {

												pthread_mutex_lock(&mutex_log); log_info(logger_pla,
														"Nivel %s: envio al personaje %c posicion de la caja %s",
														str_nivel,
														personaje->personaje,
														mensaje);
												pthread_mutex_unlock(&mutex_log);
												bool resultado =
														plan_enviarMensaje(
																str_nivel, i,
																PLA_posCajaRecurso_PER,
																mensaje);
												if (resultado)
													personaje->pos_recurso =
															sumar_valores(
																	mensaje);
												else {
													suprimir_personaje_de_estructuras(
															personaje);
												}

											}

										} else {
											suprimir_personaje_de_estructuras(
													personaje);
										}
									}
								}

								recibirMensaje(i, &tipoMensaje, &mensaje);

								pthread_mutex_lock(&mutex_log); log_info(logger_pla,
										"Nivel %s: Llego el tipo de paquete: %s .",
										str_nivel,
										obtenerNombreEnum(tipoMensaje));
								pthread_mutex_unlock(&mutex_log);
								pthread_mutex_lock(&mutex_log); log_info(logger_pla,
										"Nivel %s: Llego este mensaje: %s .",
										str_nivel, mensaje);
								pthread_mutex_unlock(&mutex_log);
							}

						}
						analizar_mensaje_rta(personaje, tipoMensaje, mensaje,
								nivel, &quantum);
						fd_personaje_actual = 0;
					} else {

						if (tipoMensaje == NIV_enemigosAsesinaron_PLA) {
							pthread_mutex_lock(&mutex_log); log_info(logger_pla,
									"Nivel %s: Recibo el asesinato de: %s",
									str_nivel, mensaje);
							pthread_mutex_unlock(&mutex_log);
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

	pthread_mutex_lock(&mutex_log); log_info(logger_pla, "Nivel %d: Sale %c de la lista de listos",
			nivel->nivel, aux->personaje);
	pthread_mutex_unlock(&mutex_log);
	imprimir_lista(LISTA_LISTOS, string_from_format("%d", nivel->nivel));

	pthread_mutex_lock(&mutex_log); log_info(logger_pla, "Nivel %d: Ahora le toca a %c moverse", nivel->nivel,
			aux->personaje);
	pthread_mutex_unlock(&mutex_log);
	return aux;
}

void tratamiento_muerte(int32_t socket_l, int32_t nivel_fd, char* mensaje,
		char* str_nivel) {

	pthread_mutex_lock(&mutex_log); log_info(logger_pla,
			"Nivel %s: El personaje del socket %d murio, se lo saca de este nivel",
			str_nivel, socket_l);
	pthread_mutex_unlock(&mutex_log);
	agregar_anormales(str_nivel, socket_l);
	pthread_mutex_lock(&mutex_log); log_info(logger_pla,
			"Nivel %s: Le aviso al nivel que el personaje %c murio por causas externas",
			str_nivel, mensaje[0]);
	pthread_mutex_unlock(&mutex_log);

	enviarMensaje(nivel_fd, PLA_personajeMuerto_NIV, mensaje);

	suprimir_de_estructuras(socket_l);

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
		//pthread_mutex_lock(&mutex_log); log_info(logger_pla, "el mensaje que mando es: %s ", prueba);
		//pthread_mutex_lock(&mutex_log); log_info(logger_pla, "la longitud del mensaje que mando es: %d ",
		//               strlen(prueba));
		pthread_mutex_lock(&mutex_log); log_info(logger_pla,
				"Nivel %d: Mando al nivel la solicitud de movimiento: %s ",
				nivel->nivel, prueba);
		pthread_mutex_unlock(&mutex_log);
		enviarMensaje(nivel->fd, PLA_movimiento_NIV, prueba);
		free(prueba);
		recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);
		if (t_mensaje == NIV_movimiento_PLA) {
			pthread_mutex_lock(&mutex_log); log_info(logger_pla,
					"Nivel %d: Mando al personaje %c la respuesta al movimiento: %s ",
					nivel->nivel, personaje->personaje, m_mensaje);
			pthread_mutex_unlock(&mutex_log);
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
				suprimir_personaje_de_estructuras(personaje);
			}
		} else {
			// Danii, aca me parece que deberia poder recibir el mensaje de que el personaje murio por enemigo, es la unica forma que se me ocurre en caso de que el personaje se mueva y el enemigo lo agarre justo, si no puedo recibir aca el mensaje de que el enemigo lo mató, entonces nunca se le va a responder que el turno estaba bien o mal, porque va a recibir el mensaje del personaje muerto por enemigo
			if (t_mensaje == NIV_enemigosAsesinaron_PLA) {
				pthread_mutex_lock(&mutex_log); log_info(logger_pla, "Nivel %d: Recibo el asesinato de: %s",
						nivel->nivel, m_mensaje);
				pthread_mutex_unlock(&mutex_log);
				tratamiento_asesinato(nivel->fd, personaje, m_mensaje,
						str_nivel);
				recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);		//matyx

				recibirMensaje(personaje->fd, &t_mensaje, &m_mensaje);	//matyx

				char * simbolo = string_new();

				string_append(&simbolo,
						string_from_format("%c", personaje->personaje));
				tratamiento_muerte(personaje->fd, nivel->fd, simbolo,
						str_nivel);
				plan_enviarMensaje(str_nivel, personaje->fd, OK1, "0");
			} else {
				suprimir_personaje_de_estructuras(personaje);
			}
		}

		free(m_mensaje);

		break;
	}
	case PER_nivelFinalizado_PLA: {
		//aviso al nivel que el pesonaje ya temino
		pthread_mutex_lock(&mutex_log); log_info(logger_pla, "Nivel %d: El personaje %c termino este nivel",
				nivel->nivel, personaje->personaje);
		pthread_mutex_unlock(&mutex_log);
		pthread_mutex_lock(&mutex_log); log_info(logger_pla,
				"Nivel %d: Mando al nivel que el personaje %c terminó este nivel ",
				nivel->nivel, personaje->personaje);
		pthread_mutex_unlock(&mutex_log);
		enviarMensaje(nivel->fd, PLA_nivelFinalizado_NIV, mensaje);
		suprimir_personaje_de_estructuras(personaje);

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
	default: {
		suprimir_personaje_de_estructuras(personaje);
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

	pthread_mutex_lock(&mutex_log); log_info(logger_pla, "Llego el tipo de paquete: %s .",
			obtenerNombreEnum(k_mensaje));
	pthread_mutex_unlock(&mutex_log);
	pthread_mutex_lock(&mutex_log); log_info(logger_pla, "Llego este mensaje: %s .", j_mensaje);
	pthread_mutex_unlock(&mutex_log);
	if (k_mensaje == PER_recurso_PLA) {

		if (!string_equals_ignore_case(j_mensaje, "0")) {

			personaje->pos_inicial = personaje->pos_recurso
					+ nivel->remain_distance;

			pthread_mutex_lock(&mutex_log); log_info(logger_pla,
					"Nivel %s: Se bloquea a %c por pedir un recurso", str_nivel,
					personaje->personaje);
			pthread_mutex_unlock(&mutex_log);
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
			pthread_mutex_lock(&mutex_log); log_info(logger_pla,
					"Nivel %d: Mando al nivel la solicitud del recurso %s ",
					nivel->nivel, pers);
			pthread_mutex_unlock(&mutex_log);
			enviarMensaje(nivel->fd, PLA_solicitudRecurso_NIV, pers);
			free(j_mensaje);
			recibirMensaje(nivel->fd, &k_mensaje, &j_mensaje);
			if (k_mensaje == NIV_recursoConcedido_PLA) {
				if (atoi(j_mensaje) == 0) { //recurso concedido
					res = 0;
					recurso_concedido(personaje, recurso, str_nivel);
					pthread_mutex_lock(&mutex_log); log_info(logger_pla,
							"Nivel %d: Al personaje %c le dimos el recurso %c",
							nivel->nivel, personaje->personaje, recurso);
					pthread_mutex_unlock(&mutex_log);
				} else {
					res = 1;
					pthread_mutex_lock(&mutex_log); log_info(logger_pla,
							"Nivel %d: El personaje %c sigue bloqueado porque no le dieron el recurso",
							nivel->nivel, personaje->personaje);
					pthread_mutex_unlock(&mutex_log);
					personaje->recurso_bloqueo = recurso;
					personaje->estoy_bloqueado = true;

				}

			} else {
				suprimir_personaje_de_estructuras(personaje);

			}
			if (string_equals_ignore_case(nivel->algol, "RR")) {
				(*quantum) = nivel->quantum;
				pthread_mutex_lock(&mutex_log); log_info(logger_pla, "Nivel %d: Valor del quantum %d",
						nivel->nivel, *quantum);
				pthread_mutex_unlock(&mutex_log);
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
					pthread_mutex_lock(&mutex_log); log_info(logger_pla,
							"Nivel %d: Se agrega al personaje %c a la lista de listos",
							nivel->nivel, personaje->personaje);
					pthread_mutex_unlock(&mutex_log);
					imprimir_lista(LISTA_LISTOS, str_nivel);
				} else {
					//lo pone al final
					pthread_mutex_lock(&mutex_listos);
					list_add(p_listos, personaje);
					pthread_mutex_unlock(&mutex_listos);
					(*quantum) = nivel->quantum;
					pthread_mutex_lock(&mutex_log); log_info(logger_pla,
							"Nivel %d: Se agrega al personaje %c a la lista de listos",
							nivel->nivel, personaje->personaje);
					pthread_mutex_unlock(&mutex_log);
					imprimir_lista(LISTA_LISTOS, str_nivel);

				}
				pthread_mutex_lock(&mutex_log); log_info(logger_pla, "Nivel %d: Valor del quantum %d",
						nivel->nivel, *quantum);
				pthread_mutex_unlock(&mutex_log);
			} else {
				pthread_mutex_lock(&mutex_listos);
				list_add(p_listos, personaje);
				pthread_mutex_unlock(&mutex_listos);
				pthread_mutex_lock(&mutex_log); log_info(logger_pla,
						"Nivel %d: Se agrega al personaje %c a la lista de listos",
						nivel->nivel, personaje->personaje);
				pthread_mutex_unlock(&mutex_log);
				imprimir_lista(LISTA_LISTOS, str_nivel);
			}
		}
	} else {
		suprimir_personaje_de_estructuras(personaje);
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
	pthread_mutex_lock(&mutex_log); log_info(logger_pla,
			"Nivel %s: Se desbloquea a %c por haber obtenido su recurso",
			str_nivel, aux->personaje);
	pthread_mutex_unlock(&mutex_log);
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
	pthread_mutex_lock(&mutex_log); log_info(logger_pla,
			"Nivel %s: Se agrega al personaje %c a la lista de listos",
			str_nivel, aux->personaje);
	pthread_mutex_unlock(&mutex_log);
	imprimir_lista(LISTA_LISTOS, str_nivel);

	bool resultado = plan_enviarMensaje(str_nivel, aux->fd, PLA_rtaRecurso_PER,
			"0");
	if (!resultado) {
		suprimir_personaje_de_estructuras(personaje);
	} else {
		pthread_mutex_lock(&mutex_log); log_info(logger_pla,
				"Nivel %s: Se responde al personaje %c que obtuvo el recurso",
				str_nivel, personaje->personaje);
		pthread_mutex_unlock(&mutex_log);
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
		//pthread_mutex_lock(&mutex_log); log_info(logger_pla, "cadena en proceso %s", recursosNuevos);
		cant = string_from_format("%d", valor->cantidad);
		string_append(&recursosNuevos, ",");
		string_append(&recursosNuevos, cant);
		//pthread_mutex_lock(&mutex_log); log_info(logger_pla, "cadena en proceso %s", recursosNuevos);
		i++;
	}
	char *cantidad = string_from_format("%d", i);
	string_append(&cantidad, ";");

	string_append(&cantidad, recursosNuevos);

	//pthread_mutex_lock(&mutex_log); log_info(logger_pla, "cadena final %s", cantidad);
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
		//pthread_mutex_lock(&mutex_log); log_info(logger_pla, "cadena en proceso %s", personajesInterb);

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
		//pthread_mutex_lock(&mutex_log); log_info(logger_pla, "cadena en proceso %s", personajesInterb);
		i++;
	}
	//pthread_mutex_lock(&mutex_log); log_info(logger_pla, "cadena final %s", personajesInterb);
	return personajesInterb;
}

void proceso_desbloqueo(t_list *recursos, int32_t fd, char *str_nivel) {
	t_list *recursosDisponibles = desbloquear_personajes(recursos, str_nivel,
			fd);
	char *recursosNuevos = transformarListaCadena(recursosDisponibles);

	if (!string_equals_ignore_case(recursosNuevos, "0;")) {
		pthread_mutex_lock(&mutex_log); log_info(logger_pla,
				"Nivel %s: Le envio al nivel los recursos disponibles %s",
				str_nivel, recursosNuevos);
		pthread_mutex_unlock(&mutex_log);
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

	t_list *p_listos = dictionary_get(listos, str_nivel);

	t_pers_por_nivel *aux = personaje;

	int i = 0;

	while (mensaje[i] != '\0') { //por si mato a mas de uno a la vez

		int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
						return nuevo->personaje == mensaje[i];
					}

		if (aux == NULL ) { //o sea que justo no lo estaba planificando

			pthread_mutex_lock(&mutex_listos);
			aux = list_remove_by_condition(p_listos, (void*) _esta_personaje);
			pthread_mutex_unlock(&mutex_listos);

			if (aux != NULL ) {
				pthread_mutex_lock(&mutex_log); log_info(logger_pla,
						"Nivel %s: Se saca a %c de la lista de listos por haber sido asesinado",
						str_nivel, aux->personaje);
				pthread_mutex_unlock(&mutex_log);
				imprimir_lista(LISTA_LISTOS, str_nivel);
			}
		} else {
			//pregunto si uno de los muertos es el planificado, si no lo es, que lo saque de listos
			if (mensaje[i] != aux->personaje) {

				pthread_mutex_lock(&mutex_listos);
				aux = list_remove_by_condition(p_listos,
						(void*) _esta_personaje);
				pthread_mutex_unlock(&mutex_listos);

				if (aux != NULL ) {
					pthread_mutex_lock(&mutex_log); log_info(logger_pla,
							"Nivel %s: Se saca a %c de la lista de listos por haber sido asesinado",
							str_nivel, aux->personaje);
					pthread_mutex_unlock(&mutex_log);
					imprimir_lista(LISTA_LISTOS, str_nivel);
				}

			}else{//si es igual al planificado que no haga nada
				pthread_mutex_lock(&mutex_log); log_info(logger_pla, "Nivel %s: El personaje %c que se estaba planificando fue asesinado",
											str_nivel, aux->personaje);
				pthread_mutex_unlock(&mutex_log);
			}
		}
			/*j = 0;
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
			 }*/
			pthread_mutex_lock(&mutex_log); log_info(logger_pla,
					"Nivel %s: Aviso al personaje %c que los enemigos lo asesinaron",
					str_nivel, aux->personaje);
			pthread_mutex_unlock(&mutex_log);
			plan_enviarMensaje(str_nivel, aux->fd, PLA_teMatamos_PER, "0");
			/*pthread_mutex_lock(&mutex_log); log_info(logger_pla,
					"Nivel %s: Aviso al nivel que los enemigos asesinaron a %c",
					str_nivel, aux->personaje);
			char* per = string_from_format("%c", personaje->personaje);
			enviarMensaje(aux->fd, PLA_perMuereInterbloqueo_NIV, per);
			 */
			suprimir_personaje_de_estructuras(aux);
			aux = NULL;

		i++;

	}

}

void planificador_analizar_mensaje(int32_t socket_r,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel) {
	char *str_nivel = string_from_format("%d", nivel->nivel);

	switch (tipoMensaje) {

	case PER_nivelFinalizado_PLA: {
		//pasamanos al nivel, lo saco de listos

		agregar_anormales(str_nivel,socket_r);

		t_list *p_listos = dictionary_get(listos, str_nivel);
		int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
			return nuevo->fd == socket_r;
		}
		pthread_mutex_lock(&mutex_listos);
		t_pers_por_nivel *aux = list_remove_by_condition(p_listos,
				(void*) _esta_personaje); //el primer elemento de la lista
		pthread_mutex_unlock(&mutex_listos);

		pthread_mutex_lock(&mutex_log); log_info(logger_pla,
				"Nivel %s: El personaje %c termino este nivel, se lo saca de listos",
				str_nivel, aux->personaje);
		pthread_mutex_unlock(&mutex_log);

		suprimir_personaje_de_estructuras(aux);

		imprimir_lista(LISTA_LISTOS, str_nivel);

		pthread_mutex_lock(&mutex_log); log_info(logger_pla,
				"Nivel %d: Informo al nivel que el personaje %c termino",
				nivel->nivel, mensaje);
		pthread_mutex_unlock(&mutex_log);
		enviarMensaje(nivel->fd, PLA_nivelFinalizado_NIV, mensaje);

		break;
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

		pthread_mutex_lock(&mutex_log); log_info(logger_pla, "Nivel %s: Recibo el asesinato de: %s", str_nivel,
				mensaje);
		pthread_mutex_unlock(&mutex_log);
		t_pers_por_nivel* personaje = NULL;
		tratamiento_asesinato(nivel->fd, personaje, mensaje, str_nivel);
		break;
	}
	case NIV_perMuereInterbloqueo_PLA: {

		//le aviso al nivel que el personaje murio
		pthread_mutex_lock(&mutex_log); log_info(logger_pla,
				"Nivel %s: Le aviso al nivel que el personaje %s murio interbloqueado",
				str_nivel, mensaje);
		pthread_mutex_unlock(&mutex_log);
		enviarMensaje(nivel->fd, PLA_perMuereInterbloqueo_NIV, mensaje);

		t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);

		int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
			return nuevo->personaje == mensaje[0];
		}

		pthread_mutex_lock(&mutex_bloqueados);
		t_pers_por_nivel *aux = list_remove_by_condition(p_bloqueados,
				(void*) _esta_personaje);
		pthread_mutex_unlock(&mutex_bloqueados);

		//le aviso al personaje que murio
		pthread_mutex_lock(&mutex_log); log_info(logger_pla,
				"Nivel %s: Le aviso al personaje %s que murio interbloqueado",
				str_nivel, mensaje);
		pthread_mutex_unlock(&mutex_log);
		plan_enviarMensaje(str_nivel, aux->fd, PLA_rtaRecurso_PER, "1");

		suprimir_personaje_de_estructuras(aux);

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

		pthread_mutex_lock(&mutex_log); log_info(logger_pla, "Nivel %s: Recibí un mensaje erroneo del fd %d",
				str_nivel, socket_r);
		pthread_mutex_unlock(&mutex_log);
		suprimir_de_estructuras(socket_r);
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
		pthread_mutex_lock(&mutex_log); log_info(logger_pla,
				"Nivel %s: Personaje desconectado, no se puede enviar el mensaje",
				str_nivel);
		pthread_mutex_unlock(&mutex_log);
		return false;
	}
}
