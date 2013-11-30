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
//extern t_dictionary *anormales;
//extern t_dictionary *monitoreo;

extern pthread_mutex_t mutex_listos;
extern pthread_mutex_t mutex_bloqueados;
//extern pthread_mutex_t mutex_monitoreo;
//extern pthread_mutex_t mutex_anormales;
extern pthread_mutex_t mutex_log;

t_config *config;
t_log* logger_pla;
extern t_list *niveles_del_sistema;

void planificador_analizar_mensaje(int32_t socket,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel, t_pers_por_nivel *personaje);
void analizar_mensaje_rta(t_pers_por_nivel *personaje,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel,
		int32_t *fd_personaje_actual, int32_t *quantum);
t_pers_por_nivel *planificar(t_niveles_sistema * str_nivel);
void recurso_concedido(t_pers_por_nivel * personaje, char recurso,
		char *str_nivel, t_niveles_sistema *nivel);
void tratamiento_recurso(t_pers_por_nivel * personaje, char* str_nivel,
		t_niveles_sistema *nivel, int32_t *quantum);
void tratamiento_asesinato(t_niveles_sistema *nivel,
		t_pers_por_nivel* personaje, char* mensaje, char* str_nivel);
void rta_movimiento(t_pers_por_nivel* personaje, char* str_nivel,
		t_niveles_sistema * nivel_fd, char* coordenadas, int32_t *quantum);
bool el_personaje_mando_fruta(t_pers_por_nivel *personaje,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel,
		int32_t *fd_personaje_actual);
void tratamiento_muerte_natural(t_pers_por_nivel *personaje, int32_t nivel_fd);

int32_t sumar_valores(char *mensaje) {

	char** n_mensaje = string_split(mensaje, ",");
	int32_t valor = atoi(n_mensaje[0]) + atoi(n_mensaje[1]);

	return valor;
}
/*
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
 */bool recibir_caja(t_pers_por_nivel *personaje, enum tipo_paquete tipoMensaje,
		char* mensaje, t_niveles_sistema *nivel) {

	bool salio_bien = true;

	char* str_nivel = nivel->str_nivel;
	pthread_mutex_lock(&mutex_log);
	log_info(logger_pla, "Nivel %s: Llego el tipo de paquete: %s .", str_nivel,
			obtenerNombreEnum(tipoMensaje));
	log_info(logger_pla, "Nivel %s: Llego este mensaje: %s .", str_nivel,
			mensaje);
	pthread_mutex_unlock(&mutex_log);

	switch (tipoMensaje) {

	case NIV_posCaja_PLA: {

		bool resultado = plan_enviarMensaje(nivel, personaje->fd,
				PLA_posCajaRecurso_PER, mensaje);
		if (resultado) {
			personaje->pos_recurso = sumar_valores(mensaje);
			pthread_mutex_lock(&mutex_log);
			log_info(logger_pla,
					"Nivel %s: envio al personaje %c posicion de la caja %s",
					str_nivel, personaje->personaje, mensaje);
			pthread_mutex_unlock(&mutex_log);
		} else {
			tratamiento_muerte_natural(personaje, nivel->fd);
			salio_bien = false;
		}
		free(mensaje);
		break;
	}
	case NIV_enemigosAsesinaron_PLA: {

		salio_bien = false; // como me mataron no tengo que seguir el curso normal

		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla, "Nivel %s: Recibo el asesinato de: %s", str_nivel,
				mensaje);
		pthread_mutex_unlock(&mutex_log);

		int i = 0;
		bool noSoyYo = true;
		t_pers_por_nivel * elMuerto = NULL;
		while (mensaje[i] != '\0') { //por si mato a mas de uno a la vez

			if (mensaje[i] == personaje->personaje) {
				noSoyYo = false;
				break;
			}

			i++;
		}

		 char * p_atacados = calloc(strlen(mensaje)+1,sizeof(char));
		strcpy(p_atacados,mensaje);

		free(mensaje);

		if (noSoyYo) {
			//o sea no mataron el que yo esperaba la respuesta, entonces vuelvo a escuchar

			tipoMensaje = ORQ_handshake_PER;
			recibirMensaje(nivel->fd, &tipoMensaje, &mensaje);

			if (tipoMensaje == ORQ_handshake_PER) {
				suprimir_de_estructuras(nivel->fd, personaje);
				free(mensaje);
				break;
			}
			recibir_caja(personaje, tipoMensaje, mensaje, nivel);
		} else {
			//aca tambien vuelvo a escuchar porque el nivel me respondio! aunque no me importa la rta

			tipoMensaje = ORQ_handshake_PER;
			recibirMensaje(nivel->fd, &tipoMensaje, &mensaje);

			if (tipoMensaje == ORQ_handshake_PER) {
				suprimir_de_estructuras(nivel->fd, personaje);
				free(mensaje);
				break;
			} else {

				pthread_mutex_lock(&mutex_log);
				log_info(logger_pla, "Nivel %s: Llego el tipo de paquete: %s.",
						str_nivel, obtenerNombreEnum(tipoMensaje));
				pthread_mutex_unlock(&mutex_log);

				if (tipoMensaje == NIV_posCaja_PLA) {
					pthread_mutex_lock(&mutex_log);
					log_info(logger_pla,
							"Nivel %s: Todo bien, llego el mensaje que esperaba.",
							str_nivel);
					pthread_mutex_unlock(&mutex_log);
				}

			}
			elMuerto = personaje;
			free(mensaje);
		}
		tratamiento_asesinato(nivel, elMuerto, p_atacados, str_nivel);
		break;
	}
	case NIV_cambiosConfiguracion_PLA: {

		//mensaje = algoritmo,quantum,retardo = "RR,4,1000"
		char** n_mensaje = string_split(mensaje, ",");
		nivel->algol = n_mensaje[0];
		nivel->quantum = atoi(n_mensaje[1]);
		nivel->retardo = atoi(n_mensaje[2]);
		log_info(logger_pla,"Nivel %s:actualizo cambios de configuracion",str_nivel);

		enviarMensaje(nivel->fd, OK1, "0");

		free(mensaje);
		tipoMensaje = ORQ_handshake_PER;
		recibirMensaje(nivel->fd, &tipoMensaje, &mensaje);

		if (tipoMensaje == ORQ_handshake_PER) {

			salio_bien = false;
			suprimir_de_estructuras(nivel->fd, personaje);
			free(mensaje);
			break;
		}

		recibir_caja(personaje, tipoMensaje, mensaje, nivel);

		break;
	}
	default: {
		tratamiento_muerte_natural(personaje, nivel->fd);
		salio_bien = false;
		free(mensaje);
	}
	}
	return salio_bien;
}

void *hilo_planificador(t_niveles_sistema *nivel) {

	int32_t miNivel = nivel->nivel;
	int32_t miFd = nivel->fd;
	char *str_nivel = nivel->str_nivel;

	logger_pla = log_create(PATH_LOG_PLA, "PLANIFICADOR", true, LOG_LEVEL_INFO);

	//inicialización
	pthread_mutex_lock(&mutex_log);
	log_info(logger_pla,
			"Hola, soy el planificador del nivel %d , mi fd es %d ", miNivel,
			miFd);
	pthread_mutex_unlock(&mutex_log);
	pthread_mutex_lock(&mutex_log);
	log_info(logger_pla,
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
	tv.tv_sec = nivel->retardo / 1000; //segundos
	tv.tv_usec = 0; //nivel->retardo * 1000; //microsegundos
	//voy metiendo aca los personajes para monitorear

	//t_list *p_monitoreo = dictionary_get(monitoreo, str_nivel);
	t_list *p_listos = dictionary_get(listos, str_nivel);

	FD_SET(nivel->fd, &master);
	fdmax = nivel->fd;
	//voy metiendo aca los personajes para monitorear

	int32_t fd_personaje_actual = 0;
	t_pers_por_nivel *personaje = NULL;
	int32_t quantum = nivel->quantum;

	// bucle principal

	for (;;) {

		//me bloqueo si no hay personajes que planificar
		if (nivel->cant_pers == 0) {
			pthread_mutex_lock(&(nivel->mutex_inicial));
		}
		//me bloqueo si no hay personajes que planificar

		///voy sacando aca los personajes anormales
		while (!list_is_empty(nivel->pers_desconectados)) {

			t_monitoreo *muerto = list_remove(nivel->pers_desconectados, 0);

			close(muerto->fd); // ¡Hasta luego!
			FD_CLR(muerto->fd, &master); // eliminar del conjunto maestro
			free(muerto);

		}
		//voy sacando aca los personajes anormales

		//voy metiendo aca los personajes para monitorear
		int a;
		for (a = 0; a < list_size(nivel->pers_conectados); a++) {

			t_monitoreo * p = list_get(nivel->pers_conectados, a);

			if (!p->monitoreado) { //pregunto si no estaba ya
				FD_SET(p->fd, &master); // añadir al conjunto maestro
				if (p->fd > fdmax) { // actualizar el máximo
					fdmax = p->fd;
				}
				p->monitoreado = true;
				//le aviso al nivel sobre el personaje nuevo
				char *valor = string_from_format("%c", p->simbolo);
				pthread_mutex_lock(&mutex_log);
										log_info(logger_pla,
												"Nivel %s: Aviso al nivel sobre el nuevo personaje en el sistema %c",
												str_nivel, p->simbolo);
										pthread_mutex_unlock(&mutex_log);
				enviarMensaje(nivel->fd, PLA_nuevoPersonaje_NIV, valor);
				free(valor);
			}
		}
		//voy metiendo aca los personajes para monitorear

		//planificar a un personaje
		if (!list_is_empty(p_listos) && (fd_personaje_actual == 0)) {
			personaje = planificar(nivel);
			bool resultado = plan_enviarMensaje(nivel, personaje->fd,
					PLA_turnoConcedido_PER, "0");
			if (resultado)
				fd_personaje_actual = personaje->fd;
		}
		//planificar a un personaje

		read_fds = master;
		if (select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1) {
			perror("select");
			exit(1);
		}

		// explorar conexiones existentes en busca de datos que leer
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				// ¡¡tenemos datos!! FD_ISSET comprueba si alguien mando algo!
				enum tipo_paquete tipoMensaje = ORQ_handshake_PER;
				char* mensaje = NULL;

				// gestionar datos del cliente del socket i!
				if (recibirMensaje(i, &tipoMensaje, &mensaje) != EXIT_SUCCESS) {

					//eliminarlo de las estructuras
					if (i == fd_personaje_actual) {
						//si lo estaba planificando tengo que liberar los recursos que tenía igual y asignarlos.

						tratamiento_muerte_natural(personaje, nivel->fd);
						fd_personaje_actual = 0;
						quantum = nivel->quantum;

					} else {
						pthread_mutex_lock(&mutex_log);
						log_info(logger_pla,
								"Nivel %s: cliente del socket %d se ha desconectado",
								str_nivel, i);
						pthread_mutex_unlock(&mutex_log);

						personaje = NULL;
						suprimir_de_estructuras(i, personaje);
					}
					//eliminarlo de las estructuras

				} else {
					// tenemos datos del cliente del socket i!
					pthread_mutex_lock(&mutex_log);
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
					pthread_mutex_unlock(&mutex_log);

					if (i == fd_personaje_actual) {

						bool entrar = el_personaje_mando_fruta(personaje,
								tipoMensaje, mensaje, nivel,
								&fd_personaje_actual);
						if (entrar) {

							tipoMensaje = ORQ_handshake_PER;
							recibirMensaje(personaje->fd, &tipoMensaje,
									&mensaje);

							if (tipoMensaje == ORQ_handshake_PER) {
								fd_personaje_actual = 0;
								entrar = false;
								suprimir_personaje_de_estructuras(personaje);
								break;

							} else {

								pthread_mutex_lock(&mutex_log);
								log_info(logger_pla,
										"Nivel %s: Llego el tipo de paquete: %s .",
										nivel->str_nivel,
										obtenerNombreEnum(tipoMensaje));
								log_info(logger_pla,
										"Nivel %s: Llego este mensaje: %s .",
										nivel->str_nivel, mensaje);
								pthread_mutex_unlock(&mutex_log);

							analizar_mensaje_rta(personaje, tipoMensaje,
									mensaje, nivel, &fd_personaje_actual,
									&quantum);
							}
						} else{
							analizar_mensaje_rta(personaje, tipoMensaje,
																mensaje, nivel, &fd_personaje_actual,
																&quantum);
						}


						fd_personaje_actual = 0;
						personaje = NULL;
					} else {

						if (tipoMensaje == NIV_enemigosAsesinaron_PLA) {
							pthread_mutex_lock(&mutex_log);
							log_info(logger_pla,
									"Nivel %s: Recibo el asesinato de: %s",
									str_nivel, mensaje);
							pthread_mutex_unlock(&mutex_log);

//							t_pers_por_nivel * elMuerto = NULL;
							int i = 0;
							while (mensaje[i] != '\0') { //por si mato a mas de uno a la vez
								if (mensaje[i] == personaje->personaje) {
									fd_personaje_actual = 0;
									break;
								}
								i++;
							}

							tratamiento_asesinato(nivel, personaje, mensaje,
									str_nivel);
							free(mensaje);

						} else

							planificador_analizar_mensaje(i, tipoMensaje,
									mensaje, nivel, personaje);
                                     fd_personaje_actual = 0;
//                                                destruir_personaje(personaje);
					}
				} // fin seccion recibir OK los datos
			} // fin de tenemos datos
		}
	} // fin bucle for principal

	pthread_exit(NULL );

}

t_pers_por_nivel *planificar(t_niveles_sistema* nivel) {
	char *str_nivel = nivel->str_nivel;
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

	pthread_mutex_lock(&mutex_log);
	log_info(logger_pla, "Nivel %d: Sale %c de la lista de listos",
			nivel->nivel, aux->personaje);
	pthread_mutex_unlock(&mutex_log);
	imprimir_lista(LISTA_LISTOS, string_from_format("%d", nivel->nivel));

	pthread_mutex_lock(&mutex_log);
	log_info(logger_pla, "Nivel %d: Ahora le toca a %c moverse", nivel->nivel,
			aux->personaje);
	pthread_mutex_unlock(&mutex_log);
	return aux;
}

bool el_personaje_mando_fruta(t_pers_por_nivel *personaje,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel,
		int32_t *fd_personaje_actual) {

	int entrar = true;
	switch (tipoMensaje) {

	case PER_posCajaRecurso_PLA: { //no consume quantum
		//pasamanos al nivel, sin procesar nada
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %s: Envio al nivel solicitud posicion caja %s",
				nivel->str_nivel, mensaje);
		pthread_mutex_unlock(&mutex_log);

		enviarMensaje(nivel->fd, PLA_posCaja_NIV, mensaje);
		free(mensaje);
		tipoMensaje = ORQ_handshake_PER;
		recibirMensaje(nivel->fd, &tipoMensaje, &mensaje);

		if (tipoMensaje == ORQ_handshake_PER) {
			*fd_personaje_actual = 0;
			suprimir_personaje_de_estructuras(personaje);
			entrar = false;
			break;
		}
		if (tipoMensaje == NIV_enemigosAsesinaron_PLA) {
			int i = 0;
			while (mensaje[i] != '\0') { //por si mato a mas de uno a la vez
				if (mensaje[i] == personaje->personaje) { //si me mato a mi
					*fd_personaje_actual = 0;
					entrar = false;
					break;
				}
				i++;
			}

		}
		entrar = recibir_caja(personaje, tipoMensaje, mensaje, nivel);

		break;
	}
	default: {

		//tratamiento_muerte_natural(personaje, nivel->fd);
		entrar = false;
		break;
	}
	}
	return entrar;
}

void tratamiento_muerte_natural(t_pers_por_nivel *personaje, int32_t nivel_fd) {

	int32_t _esta_nivel(t_niveles_sistema *nuevo) {
		return nuevo->fd == nivel_fd;
	}

	t_niveles_sistema *niv = list_find(niveles_del_sistema,
			(void*) _esta_nivel);

	pthread_mutex_lock(&mutex_log);
	log_info(logger_pla,
			"Nivel %d: Le aviso al nivel que el personaje %c murio por causas externas",
			niv->nivel, personaje->personaje);
	pthread_mutex_unlock(&mutex_log);
	char* muerto = string_from_format("%c", personaje->personaje);
	enviarMensaje(nivel_fd, PLA_perMuereNaturalmente_NIV, muerto);
	suprimir_personaje_de_estructuras(personaje);
	free(muerto);
}

void tratamiento_muerte(int32_t socket_l, int32_t nivel_fd, char* mensaje,
		char* str_nivel) {

	pthread_mutex_lock(&mutex_log);
	log_info(logger_pla,
			"Nivel %s: El personaje del socket %d murio, se lo saca de este nivel",
			str_nivel, socket_l);
	pthread_mutex_unlock(&mutex_log);
//agregar_anormales(str_nivel, socket_l);
	pthread_mutex_lock(&mutex_log);
	log_info(logger_pla,
			"Nivel %s: Le aviso al nivel que el personaje %c murio por causas externas",
			str_nivel, mensaje[0]);
	pthread_mutex_unlock(&mutex_log);

	enviarMensaje(nivel_fd, PLA_perMuereNaturalmente_NIV, mensaje);
	t_pers_por_nivel* v = NULL;
	suprimir_de_estructuras(socket_l, v);

}

void posibles_respuestas_del_nivel(t_pers_por_nivel *personaje,
		t_niveles_sistema *nivel, int32_t *quantum,
		int32_t *fd_personaje_actual) {
	char *str_nivel = nivel->str_nivel;

	enum tipo_paquete tipoMensaje = ORQ_handshake_PER;
	char* mensaje = NULL;

	recibirMensaje(nivel->fd, &tipoMensaje, &mensaje);

	pthread_mutex_lock(&mutex_log);
	log_info(logger_pla, "Nivel %s: Llego el tipo de paquete: %s .", str_nivel,
			obtenerNombreEnum(tipoMensaje));
	log_info(logger_pla, "Nivel %s: Llego este mensaje: %s .", str_nivel,
			mensaje);
	pthread_mutex_unlock(&mutex_log);

	switch (tipoMensaje) {

	case NIV_movimiento_PLA: {
		rta_movimiento(personaje, str_nivel, nivel, mensaje, quantum);

		break;
	}

	case NIV_enemigosAsesinaron_PLA: {
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla, "Nivel %d: Recibo el asesinato de: %s",
				nivel->nivel, mensaje);
		pthread_mutex_unlock(&mutex_log);

		//tratamiento_asesinato(nivel, personaje, mensaje, str_nivel);

		int i = 0;
		bool noSoyYo = true;
		t_pers_por_nivel * elMuerto = NULL;
		while (mensaje[i] != '\0') { //por si mato a mas de uno a la vez

			if (mensaje[i] == personaje->personaje) {
				noSoyYo = false;
				break;
			}

			i++;
		}

		if (noSoyYo) {
			//o sea no mataron el que yo esperaba la respuesta, entonces vuelvo a escuchar
			free(mensaje);
			posibles_respuestas_del_nivel(personaje, nivel, quantum,
					fd_personaje_actual);
		} else {
			//aca tambien vuelvo a escuchar porque el nivel me respondio! aunque no me importa la rta
			*quantum = nivel->quantum;

			tipoMensaje = ORQ_handshake_PER;
			char* O_mensaje = NULL;
			recibirMensaje(nivel->fd, &tipoMensaje, &O_mensaje);

			if (tipoMensaje == ORQ_handshake_PER) {
				suprimir_de_estructuras(nivel->fd, personaje);
				break;
			}

			pthread_mutex_lock(&mutex_log);
			log_info(logger_pla, "Nivel %s: Llego el tipo de paquete: %s.",
					str_nivel, obtenerNombreEnum(tipoMensaje));
			pthread_mutex_unlock(&mutex_log);
			if (tipoMensaje == NIV_movimiento_PLA) {
				pthread_mutex_lock(&mutex_log);
				log_info(logger_pla,
						"Nivel %s: Todo bien, llego el mensaje que esperaba.",
						str_nivel);
				pthread_mutex_unlock(&mutex_log);
			}
			free(O_mensaje);

			elMuerto = personaje;

		}
		tratamiento_asesinato(nivel, elMuerto, mensaje, str_nivel);
		free(mensaje);

		/*recibirMensaje(nivel->fd, &tipoMensaje, &mensaje);		//matyx
		 recibirMensaje(personaje->fd, &tipoMensaje, &mensaje);	//matyx
		 char * simbolo = string_new();

		 string_append(&simbolo, string_from_format("%c", personaje->personaje));
		 tratamiento_muerte(personaje->fd, nivel->fd, simbolo, str_nivel);
		 plan_enviarMensaje(str_nivel, personaje->fd, OK1, "0");*/
		break;
	}
	case NIV_cambiosConfiguracion_PLA: {
		//mensaje = algoritmo,quantum,retardo = "RR,4,1000"
		char** n_mensaje = string_split(mensaje, ",");
		nivel->algol = n_mensaje[0];
		nivel->quantum = atoi(n_mensaje[1]);
		nivel->retardo = atoi(n_mensaje[2]);

		log_info(logger_pla,"Nivel %s:actualizo cambios de configuracion",str_nivel);

		free(mensaje);
		enviarMensaje(nivel->fd, OK1, "0");

		posibles_respuestas_del_nivel(personaje, nivel, quantum,
				fd_personaje_actual);
		break;
	}
	case NIV_perMuereInterbloqueo_PLA: {

		t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);

		int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
			return nuevo->personaje == mensaje[0];
		}

		pthread_mutex_lock(&mutex_bloqueados);
		t_pers_por_nivel *aux = list_remove_by_condition(p_bloqueados,
				(void*) _esta_personaje);
		pthread_mutex_unlock(&mutex_bloqueados);

		int32_t fd_prueba = aux->fd;

		//le aviso al nivel que el personaje murio
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %s: Le aviso al nivel que el personaje %s murio interbloqueado",
				str_nivel, mensaje);
		pthread_mutex_unlock(&mutex_log);
		enviarMensaje(nivel->fd, PLA_perMuereInterbloqueo_NIV, mensaje);
		//le aviso al nivel que el personaje murio

		//le aviso al personaje que murio
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %s: Le aviso al personaje %s que murio interbloqueado",
				str_nivel, mensaje);
		pthread_mutex_unlock(&mutex_log);
		suprimir_personaje_de_estructuras(aux);
		plan_enviarMensaje(nivel, fd_prueba, PLA_rtaRecurso_PER, "1");
		//le aviso al personaje que murio

		//vuelvo a escuchar porque a mi no me afecta, si murio alguien era porque estaba en listos
		free(mensaje);
		posibles_respuestas_del_nivel(personaje, nivel, quantum,
				fd_personaje_actual);

		break;
	}
	default: {
		//aca se pudo haber caido el nivel?
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla, "Nivel %s: se cayo el nivel ", str_nivel);
		pthread_mutex_unlock(&mutex_log);

		/*
		 log_info(logger_pla,
		 "Nivel %s: Le aviso al nivel que el personaje %c murio por causas externas",
		 str_nivel, personaje->personaje);

		 char* muerto = string_from_format("%c", personaje->fd);
		 enviarMensaje(nivel->fd, PLA_perMuereNaturalmente_NIV, muerto);
		 suprimir_personaje_de_estructuras(personaje);
		 *quantum = nivel->quantum;
		 *quantum */
		suprimir_de_estructuras(nivel->fd, personaje);
		free(mensaje);
		break;
	}

	}
}

void rta_movimiento(t_pers_por_nivel* personaje, char* str_nivel,
		t_niveles_sistema * nivel, char* coordenadas, int32_t *quantum) {

	if (personaje != NULL ) {

		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %s: Mando al personaje %c la respuesta al movimiento: %s ",
				str_nivel, personaje->personaje, coordenadas);
		pthread_mutex_unlock(&mutex_log);
		bool resultado = plan_enviarMensaje(nivel, personaje->fd,
				PLA_movimiento_PER, coordenadas);

		if (resultado) {
			tratamiento_recurso(personaje, str_nivel, nivel, quantum);

		} else {
			tratamiento_muerte_natural(personaje, nivel->fd);

		}
	}

}

void analizar_mensaje_rta(t_pers_por_nivel *personaje,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel,
		int32_t *fd_personaje_actual, int32_t *quantum) {
	//char *str_nivel = nivel->str_nivel;

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

		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %d: Mando al nivel la solicitud de movimiento: %s ",
				nivel->nivel, prueba);
		pthread_mutex_unlock(&mutex_log);
		enviarMensaje(nivel->fd, PLA_movimiento_NIV, prueba);
		free(prueba);

		posibles_respuestas_del_nivel(personaje, nivel, quantum,
				fd_personaje_actual);

		break;
	}
	case PER_nivelFinalizado_PLA: {
		//aviso al nivel que el pesonaje ya temino
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla, "Nivel %d: El personaje %c termino este nivel",
				nivel->nivel, personaje->personaje);
		pthread_mutex_unlock(&mutex_log);
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %d: Mando al nivel que el personaje %c terminó este nivel ",
				nivel->nivel, personaje->personaje);
		pthread_mutex_unlock(&mutex_log);
		enviarMensaje(nivel->fd, PLA_nivelFinalizado_NIV, mensaje);

		personaje->borrame = false;
		suprimir_personaje_de_estructuras(personaje);

		break;
	}

	default: {
		tratamiento_muerte_natural(personaje, nivel->fd);
		break;
	}
	}
	free(mensaje);

}

void personaje_pidio_recurso(t_pers_por_nivel * personaje,
		t_niveles_sistema * nivel, char *recurso_pedido) {

	char* str_nivel = string_from_format("%d", nivel->nivel);

	t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);
	personaje->pos_inicial = personaje->pos_recurso + nivel->remain_distance;
	char recurso = recurso_pedido[0];
	personaje->recurso_bloqueo = recurso;

	pthread_mutex_lock(&mutex_log);
	log_info(logger_pla, "Nivel %s: Se bloquea a %c por pedir un recurso",
			str_nivel, personaje->personaje);
	pthread_mutex_unlock(&mutex_log);
	pthread_mutex_lock(&mutex_bloqueados);
	list_add(p_bloqueados, personaje);
	pthread_mutex_unlock(&mutex_bloqueados);
	imprimir_lista(LISTA_BLOQUEADOS, str_nivel);

	char * pers = string_from_format("%c", personaje->personaje);
	string_append(&pers, ",");
	string_append(&pers, recurso_pedido);

	pthread_mutex_lock(&mutex_log);
	log_info(logger_pla,
			"Nivel %d: Mando al nivel la solicitud del recurso %s ",
			nivel->nivel, pers);
	pthread_mutex_unlock(&mutex_log);

	enviarMensaje(nivel->fd, PLA_solicitudRecurso_NIV, pers);
	free(pers);

}

void nivel_otorga_recurso_o_no(t_pers_por_nivel *personaje,
		t_niveles_sistema *nivel) {
	char *str_nivel = nivel->str_nivel;
	enum tipo_paquete tipoMensaje = ORQ_handshake_PER;
	char* mensaje = NULL;
	recibirMensaje(nivel->fd, &tipoMensaje, &mensaje);
	switch (tipoMensaje) {
	case NIV_recursoConcedido_PLA: {
		if (atoi(mensaje) == 0) { //recurso concedido

			pthread_mutex_lock(&mutex_log);
			log_info(logger_pla,
					"Nivel %d: Al personaje %c le dimos el recurso %c",
					nivel->nivel, personaje->personaje,
					personaje->recurso_bloqueo);
			pthread_mutex_unlock(&mutex_log);

			recurso_concedido(personaje, personaje->recurso_bloqueo, str_nivel,
					nivel);

			personaje->estoy_bloqueado = false;
		} else {

			pthread_mutex_lock(&mutex_log);
			log_info(logger_pla,
					"Nivel %d: El personaje %c sigue bloqueado porque no le dieron el recurso",
					nivel->nivel, personaje->personaje);
			pthread_mutex_unlock(&mutex_log);

			personaje->estoy_bloqueado = true;

		}
		break;
	}
	default: {
		//aca se pudo haber caido el nivel?
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla, "Nivel %s: se cayo el nivel", str_nivel);
		pthread_mutex_unlock(&mutex_log);

		/*pthread_mutex_lock(&mutex_log);
		 log_info(logger_pla,
		 "Nivel %s: Le aviso al nivel que el personaje %c murio por causas externas",
		 str_nivel, personaje->personaje);
		 pthread_mutex_unlock(&mutex_log);
		 char* muerto = string_from_format("%c", personaje->fd);
		 enviarMensaje(nivel->fd, PLA_personajeMuerto_NIV, muerto);*/
		suprimir_de_estructuras(nivel->fd, personaje);
		free(mensaje);
		break;
	}
	}
}

void tratamiento_recurso(t_pers_por_nivel * personaje, char* str_nivel,
		t_niveles_sistema *nivel, int32_t *quantum) {
//pasamanos al nivel, lo paso de listos a bloqueados

	t_list *p_listos = dictionary_get(listos, str_nivel);

	enum tipo_paquete tipoMensaje = ORQ_handshake_PER;
	char* mensaje = NULL;
	recibirMensaje(personaje->fd, &tipoMensaje, &mensaje);

	if (tipoMensaje == PER_recurso_PLA) {

		if (!string_equals_ignore_case(mensaje, "0")) {
			//el flaco pidio un recurso

			pthread_mutex_lock(&mutex_log);
			log_info(logger_pla, "Nivel %s: Llego el tipo de paquete: %s.",
					str_nivel, obtenerNombreEnum(tipoMensaje));
			pthread_mutex_unlock(&mutex_log);
			pthread_mutex_lock(&mutex_log);
			log_info(logger_pla, "Nivel %s: Llego este mensaje: %s.", str_nivel,
					mensaje);
			pthread_mutex_unlock(&mutex_log);

			personaje_pidio_recurso(personaje, nivel, mensaje);
			free(mensaje);

			nivel_otorga_recurso_o_no(personaje, nivel);

			if (string_equals_ignore_case(nivel->algol, "RR")) {
				(*quantum) = nivel->quantum;
				pthread_mutex_lock(&mutex_log);
				log_info(logger_pla, "Nivel %d: Valor del quantum %d",
						nivel->nivel, *quantum);
				pthread_mutex_unlock(&mutex_log);
			}

		} else {

			//no pidio un recurso
			if (string_equals_ignore_case(nivel->algol, "RR")) {
				(*quantum)--;
				if ((*quantum) != 0) {
					pthread_mutex_lock(&mutex_listos);
					//lo pone primero asi despues sigue el mismo planificandose
					list_add_in_index(p_listos, 0, personaje);
					pthread_mutex_unlock(&mutex_listos);
					pthread_mutex_lock(&mutex_log);
					log_info(logger_pla,
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
					pthread_mutex_lock(&mutex_log);
					log_info(logger_pla,
							"Nivel %d: Se agrega al personaje %c a la lista de listos",
							nivel->nivel, personaje->personaje);
					pthread_mutex_unlock(&mutex_log);
					imprimir_lista(LISTA_LISTOS, str_nivel);

				}
				pthread_mutex_lock(&mutex_log);
				log_info(logger_pla, "Nivel %d: Valor del quantum %d",
						nivel->nivel, *quantum);
				pthread_mutex_unlock(&mutex_log);
			} else {
				pthread_mutex_lock(&mutex_listos);
				list_add(p_listos, personaje);
				pthread_mutex_unlock(&mutex_listos);
				pthread_mutex_lock(&mutex_log);
				log_info(logger_pla,
						"Nivel %d: Se agrega al personaje %c a la lista de listos",
						nivel->nivel, personaje->personaje);
				pthread_mutex_unlock(&mutex_log);
				imprimir_lista(LISTA_LISTOS, str_nivel);
			}
		}
	} else {
		tratamiento_muerte_natural(personaje, nivel->fd);
	}

}
void recurso_concedido(t_pers_por_nivel * personaje, char recurso,
		char *str_nivel, t_niveles_sistema *nivel) {
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
	pthread_mutex_lock(&mutex_log);
	log_info(logger_pla,
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
	pthread_mutex_lock(&mutex_log);
	log_info(logger_pla,
			"Nivel %s: Se agrega al personaje %c a la lista de listos",
			str_nivel, aux->personaje);
	pthread_mutex_unlock(&mutex_log);
	imprimir_lista(LISTA_LISTOS, str_nivel);

	bool resultado = plan_enviarMensaje(nivel, aux->fd, PLA_rtaRecurso_PER,
			"0");
	if (!resultado) {
		tratamiento_muerte_natural(personaje, nivel->fd);
	} else {
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
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
		//log_info(logger_pla, "cadena en proceso %s", recursosNuevos);
		cant = string_from_format("%d", valor->cantidad);
		string_append(&recursosNuevos, ",");
		string_append(&recursosNuevos, cant);
		// log_info(logger_pla, "cadena en proceso %s", recursosNuevos);
		i++;
	}
	char *cantidad = string_from_format("%d", i);
	string_append(&cantidad, ";");

	string_append(&cantidad, recursosNuevos);

// log_info(logger_pla, "cadena final %s", cantidad);
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
		// log_info(logger_pla, "cadena en proceso %s", personajesInterb);

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
		// log_info(logger_pla, "cadena en proceso %s", personajesInterb);
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
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
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

void tratamiento_asesinato(t_niveles_sistema *nivel,
		t_pers_por_nivel* personaje, char* mensaje, char* str_nivel) {

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
				pthread_mutex_lock(&mutex_log);
				log_info(logger_pla,
						"Nivel %s: Se saca a %c de la lista de listos por haber sido asesinado",
						str_nivel, aux->personaje);
				pthread_mutex_unlock(&mutex_log);
				imprimir_lista(LISTA_LISTOS, str_nivel);
			}else{
				pthread_mutex_lock(&mutex_log);
				log_info(logger_pla,"Nivel %s: No se encontró este personaje (%c) en la lista de listos, entonces no lo pude remover",str_nivel, mensaje[i]); // (matyx) Dani fijate como resolver esto
				pthread_mutex_unlock(&mutex_log);
			}
		} else {
			//pregunto si uno de los muertos es el planificado, si no lo es, que lo saque de listos
			if (mensaje[i] != aux->personaje) {

				pthread_mutex_lock(&mutex_listos);
				aux = list_remove_by_condition(p_listos,
						(void*) _esta_personaje);
				pthread_mutex_unlock(&mutex_listos);

				if (aux != NULL ) {
					pthread_mutex_lock(&mutex_log);
					log_info(logger_pla,
							"Nivel %s: Se saca a %c de la lista de listos por haber sido asesinado",
							str_nivel, aux->personaje);
					pthread_mutex_unlock(&mutex_log);
					imprimir_lista(LISTA_LISTOS, str_nivel);
				}

			} else { //si es igual al planificado que no haga nada
				pthread_mutex_lock(&mutex_log);
				log_info(logger_pla,
						"Nivel %s: El personaje %c que se estaba planificando fue asesinado",
						str_nivel, aux->personaje);
				pthread_mutex_unlock(&mutex_log);
			}
		}

		if (aux != NULL ) {
			log_info(logger_pla,
					"Nivel %s: Aviso al nivel que los enemigos asesinaron a %c",
					str_nivel, aux->personaje);
			char* per = string_from_format("%c", personaje->personaje);
			enviarMensaje(nivel->fd, PLA_personajeMuerto_NIV, per);

			int32_t fd_prueba = aux->fd;

			pthread_mutex_lock(&mutex_log);
			log_info(logger_pla,
					"Nivel %s: Aviso al personaje %c que los enemigos lo asesinaron",
					str_nivel, aux->personaje);
			pthread_mutex_unlock(&mutex_log);
			plan_enviarMensaje(nivel, fd_prueba, PLA_teMatamos_PER, "0");

			suprimir_personaje_de_estructuras(aux);
			aux = NULL;
		}
		i++;

	}

}

void planificador_analizar_mensaje(int32_t socket_r,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel, 
		t_pers_por_nivel *personaje) {
	char *str_nivel = nivel->str_nivel;

	switch (tipoMensaje) {

	case PER_nivelFinalizado_PLA: {
		//pasamanos al nivel, lo saco de listos

		//agregar_anormales(str_nivel,socket_r);

		t_list *p_listos = dictionary_get(listos, str_nivel);
		int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
			return nuevo->fd == socket_r;
		}
		pthread_mutex_lock(&mutex_listos);
		t_pers_por_nivel *aux = list_remove_by_condition(p_listos,
				(void*) _esta_personaje); //el primer elemento de la lista
		pthread_mutex_unlock(&mutex_listos);

		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %s: El personaje %c termino este nivel, se lo saca de listos",
				str_nivel, aux->personaje);
		pthread_mutex_unlock(&mutex_log);

		suprimir_personaje_de_estructuras(aux);

		imprimir_lista(LISTA_LISTOS, str_nivel);

		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
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
		log_info(logger_pla,"Nivel %s:actualizo cambios de configuracion",str_nivel);


		enviarMensaje(nivel->fd, OK1, "0");


		break;
	}
	case NIV_enemigosAsesinaron_PLA: {

		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla, "Nivel %s: Recibo el asesinato de: %s", str_nivel,
				mensaje);
		pthread_mutex_unlock(&mutex_log);
		//t_pers_por_nivel* personaje = NULL;
		tratamiento_asesinato(nivel, personaje, mensaje, str_nivel);
		break;
	}
	case NIV_perMuereInterbloqueo_PLA: {

		//le aviso al nivel que el personaje murio
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
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

		int32_t fd_prueba = aux->fd;

		//le aviso al personaje que murio
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %s: Le aviso al personaje %s que murio interbloqueado",
				str_nivel, mensaje);
		pthread_mutex_unlock(&mutex_log);
		suprimir_personaje_de_estructuras(aux);
		plan_enviarMensaje(nivel, fd_prueba, PLA_rtaRecurso_PER, "1");

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
	case NIV_movimiento_PLA: {
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla, "Nivel %s: salve el error ", str_nivel);
		pthread_mutex_unlock(&mutex_log);
		break;
	}
	default: {

		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla, "Nivel %s: Recibí un mensaje erroneo del fd %d",
				str_nivel, socket_r);
		pthread_mutex_unlock(&mutex_log);
		t_pers_por_nivel* v = NULL;
		suprimir_de_estructuras(socket_r, v);
		break;
	}
	}

	free(mensaje);
}

bool plan_enviarMensaje(t_niveles_sistema* nivel, int32_t fd,
		enum tipo_paquete paquete, char* mensaje) {

	t_monitoreo *fd_muerto = NULL;
	int32_t _esta_muerto(t_monitoreo *nuevo) {
		return nuevo->fd == fd;
	}

	fd_muerto = list_find(nivel->pers_desconectados, (void*) _esta_muerto);

	if (fd_muerto == NULL ) {
		if (enviarMensaje(fd, paquete, mensaje) != EXIT_SUCCESS)
			return false;
		else
			return true;
	} else {
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %s: Personaje desconectado, no se puede enviar el mensaje",
				nivel->str_nivel);
		pthread_mutex_unlock(&mutex_log);
		return false;
	}
}
