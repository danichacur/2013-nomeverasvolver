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
//extern t_dictionary *anormales;
extern t_dictionary *monitoreo;
extern t_list *personajes_para_koopa; // es para lanzar koopa

extern pthread_mutex_t mutex_listos;
extern pthread_mutex_t mutex_bloqueados;
extern pthread_mutex_t mutex_monitoreo;
extern pthread_mutex_t mutex_personajes_para_koopa;

extern char *ruta_koopa;
extern char *ruta_script;
extern char *ruta_disco;

t_config *config;
t_log* logger_pla;

void planificador_analizar_mensaje(int32_t socket,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel);
void analizar_mensaje_rta(t_pers_por_nivel *personaje,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel,
		int32_t *quantum);
t_pers_por_nivel *planificar(t_niveles_sistema * str_nivel);

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
	char* cadena = "PLANIFICADOR_Nivel_";
	char * nueva_cadena = calloc(strlen("PLANIFICADOR_Nivel: "), sizeof(char));
	strcpy(nueva_cadena, cadena);
	string_append(&nueva_cadena, str_nivel);
	//inicialización
	logger_pla = log_create(PATH_LOG_PLA, nueva_cadena, true, LOG_LEVEL_INFO);
	//inicialización
	log_info(logger_pla,
			"hola, soy el planificador del nivel %d , mi fd es %d ", miNivel,
			miFd);
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
			enviarMensaje(personaje->fd, PLA_turnoConcedido_PER, "0");
			fd_personaje_actual = personaje->fd;
		}
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

					//todo: que libere los recursos y haga gestion de los bloqueados!
					//si era el que estaba planificando liberar los recursos
					//y salir del for asi planifica a alguien mas

					//eliminarlo de las estructuras
					if (i == fd_personaje_actual) {
						//si lo estaba planificando tengo que liberar los recursos que tenía igual y asignarlos.

						proceso_desbloqueo(personaje->recursos_obtenidos,
								nivel->fd, str_nivel);

					}
					supr_pers_de_estructuras(i);
					//eliminarlo de las estructuras

					close(i); // ¡Hasta luego!
					FD_CLR(i, &master); // eliminar del conjunto maestro
					break;

					if (i == fd_personaje_actual)
						break;

				} else {
					// tenemos datos del cliente del socket i!
					log_info(logger_pla, "Llego el tipo de paquete: %s .",
							obtenerNombreEnum(tipoMensaje));
					log_info(logger_pla, "Llego este mensaje: %s .", mensaje);
					log_info(logger_pla, "se esperaba la respuesta de %d .",
							fd_personaje_actual);
					log_info(logger_pla, "se recibió el mensaje de %d .", i);

					if (i == fd_personaje_actual) {
						if (tipoMensaje == PER_posCajaRecurso_PLA) { //no consume quantum
							//pasamanos al nivel, sin procesar nada
							log_info(logger_pla,
									"envio al nivel solicitud pos caja %s",
									mensaje);
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
										"envio al personaje pos caja %s",
										mensaje);
								enviarMensaje(i, PLA_posCajaRecurso_PER,
										mensaje);
								personaje->pos_recurso = sumar_valores(mensaje);

							} else
								supr_pers_de_estructuras(i);

							recibirMensaje(i, &tipoMensaje, &mensaje);

							log_info(logger_pla, "Llego el tipo de paquete: %s .",
									obtenerNombreEnum(tipoMensaje));
							log_info(logger_pla, "Llego este mensaje: %s .",
									mensaje);
						}

						analizar_mensaje_rta(personaje, tipoMensaje, mensaje,
								nivel, &quantum);
						fd_personaje_actual = 0;
					} else {
						planificador_analizar_mensaje(i, tipoMensaje, mensaje,
								nivel);
//						fd_personaje_actual = 0;
//						destruir_personaje(personaje);
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

			return primero < segundo;
		}

		list_sort(p_listos, (void*) _menor_distancia);
	}

	pthread_mutex_lock(&mutex_listos);
	aux = list_remove(p_listos, 0); //el primer elemento de la lista
	pthread_mutex_unlock(&mutex_listos);

	log_info(logger_pla, "ahora le toca a %c moverse", aux->personaje);
	return aux;
}

void tratamiento_muerte(int32_t socket_l, int32_t nivel_fd, char* mensaje,
		char* str_nivel) {

	log_info(logger_pla, "le aviso al nivel que el personaje %c murio ",
			mensaje[0]);
	log_info(logger_pla, "yo planificador debería liberar recursos? SI ");
	enviarMensaje(nivel_fd, PLA_personajeMuerto_NIV, mensaje);

	t_list *p_listos = dictionary_get(listos, str_nivel);

	int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
		return nuevo->fd == socket_l;
	}
	pthread_mutex_lock(&mutex_listos);
	t_pers_por_nivel *aux = list_remove_by_condition(p_listos,
			(void*) _esta_personaje); //el primer elemento de la lista
	pthread_mutex_unlock(&mutex_listos);
	log_info(logger_pla, "el personaje %c murio, se lo saca de este nivel %s",
			aux->personaje, str_nivel);
	supr_pers_de_estructuras(socket_l);
	free(aux);

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
		log_info(logger_pla, "el mensaje que mando es: %s ", prueba);
		log_info(logger_pla, "la longitud del mensaje que mando es: %d ",
				strlen(prueba));

		enviarMensaje(nivel->fd, PLA_movimiento_NIV, prueba);
		free(prueba);
		recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);
		if (t_mensaje == NIV_movimiento_PLA) {
			enviarMensaje(personaje->fd, PLA_movimiento_PER, m_mensaje);
			int res = tratamiento_recurso(personaje,str_nivel,nivel,quantum);

			if (res == 1){
				free(m_mensaje);
				break;
			}
		} else {
			supr_pers_de_estructuras(personaje->fd);
			break;
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
		log_info(logger_pla, "el personaje %c termino este nivel %d",
				personaje->personaje, nivel->nivel);
		proceso_desbloqueo(personaje->recursos_obtenidos, nivel->fd, str_nivel);
		destruir_personaje(personaje);

		enviarMensaje(nivel->fd, PLA_nivelFinalizado_NIV, mensaje);
		/*                enum tipo_paquete t_mensaje;
		 char* m_mensaje = NULL;
		 recibirMensaje(nivel->fd, &t_mensaje, &m_mensaje);
		 */break;
	}
	default: {
		log_info(logger_pla, "el personaje %c se desconecto ",
				personaje->personaje);
		supr_pers_de_estructuras(personaje->fd);
		break;
	}
	}
	free(mensaje);

}
void recurso_concedido(t_pers_por_nivel * personaje,char recurso, char *str_nivel);
int tratamiento_recurso(t_pers_por_nivel * personaje, char* str_nivel,
		t_niveles_sistema *nivel, int32_t *quantum) {
int res =0;
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

			personaje->pos_inicial = personaje->pos_recurso;

			log_info(logger_pla, "se bloquea a %c por pedir un recurso",
					personaje->personaje);
			pthread_mutex_lock(&mutex_bloqueados);
			list_add(p_bloqueados, personaje);
			pthread_mutex_unlock(&mutex_bloqueados);

			char recurso = j_mensaje[0];
			int32_t _esta_recurso(t_recursos_obtenidos *nuevo) {
				return nuevo->recurso == recurso;
			}
			char * pers = string_from_format("%c", personaje->personaje);
			string_append(&pers, ",");
			string_append(&pers, j_mensaje);
			enviarMensaje(nivel->fd, PLA_solicitudRecurso_NIV, pers);
			free(j_mensaje);
			recibirMensaje(nivel->fd, &k_mensaje, &j_mensaje);
			if (k_mensaje == NIV_recursoConcedido_PLA) {
				if (atoi(j_mensaje) == 0) { //recurso concedido
					res = 0;
					recurso_concedido(personaje,recurso, str_nivel);

				} else {
					res = 1;
					log_info(logger_pla,
							"el personaje %c sigue bloqueado porque no le dieron el recurso",
							personaje->personaje);
					personaje->recurso_bloqueo = recurso;
					personaje->estoy_bloqueado = true;

				}

			} else
				supr_pers_de_estructuras(personaje->fd);
			if (string_equals_ignore_case(nivel->algol, "RR")) {
				(*quantum) = nivel->quantum;
			}
			free(j_mensaje);
		}else{

			//modificar que si pidieron un recurso no se entre aca
					if (string_equals_ignore_case(nivel->algol, "RR")) {
						(*quantum)--;
						if ((*quantum) != 0) {
							pthread_mutex_lock(&mutex_listos);
							//lo pone primero asi despues sigue el mismo planificandose
							list_add_in_index(p_listos, 0, personaje);
							pthread_mutex_unlock(&mutex_listos);
						} else {
							//lo pone al final
							pthread_mutex_lock(&mutex_listos);
							list_add(p_listos, personaje);
							pthread_mutex_unlock(&mutex_listos);
							(*quantum) = nivel->quantum;
						}
					} else {
						pthread_mutex_lock(&mutex_listos);
						list_add(p_listos, personaje);
						pthread_mutex_unlock(&mutex_listos);
					}

		}
	} else
		supr_pers_de_estructuras(personaje->fd);
return res;
}
void recurso_concedido(t_pers_por_nivel * personaje,char recurso, char *str_nivel) {
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
	log_info(logger_pla, "se desbloquea a %c por haber obtenido su recurso",
			aux->personaje);

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

	//
	enviarMensaje(aux->fd, PLA_rtaRecurso_PER, "0");

}

char * transformarListaCadena(t_list *recursosDisponibles) {
	char *recursosNuevos = string_new(); //"F,1;T,6;M,12";
	char *recu, *cant;
	int i=0;
	while (!list_is_empty(recursosDisponibles)) {
		t_recursos_obtenidos * valor = list_remove(recursosDisponibles, 0);
		recu = string_from_format("%c", valor->recurso);
		if (i != 0)
			string_append(&recursosNuevos, ";");
		string_append(&recursosNuevos, recu);
		log_info(logger_pla, "cadena en proceso %s", recursosNuevos);
		cant = string_from_format("%d", valor->cantidad);
		string_append(&recursosNuevos, ",");
		string_append(&recursosNuevos, cant);
		log_info(logger_pla, "cadena en proceso %s", recursosNuevos);
		i++;
	}
	char *cantidad = string_from_format("%d",i);
	string_append(&cantidad, ";");

	string_append(&cantidad, recursosNuevos);

	log_info(logger_pla, "cadena final %s", cantidad);
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
		log_info(logger_pla, "cadena en proceso %s", personajesInterb);

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
		log_info(logger_pla, "cadena en proceso %s", personajesInterb);
		i++;
	}
	log_info(logger_pla, "cadena final %s", personajesInterb);
	return personajesInterb;
}

void proceso_desbloqueo(t_list *recursos, int32_t fd, char *str_nivel) {
	t_list *recursosDisponibles = desbloquear_personajes(recursos, str_nivel,
			fd);
	char *recursosNuevos = transformarListaCadena(recursosDisponibles);

	if(!string_equals_ignore_case(recursosNuevos,"0"))
		enviarMensaje(fd, PLA_actualizarRecursos_NIV, recursosNuevos);

	int m;
	t_recursos_obtenidos *elem;
	for (m = 0; !list_is_empty(recursosDisponibles); m++) {
		elem = list_remove(recursosDisponibles, m);
		free(elem);
	}

	list_destroy(recursosDisponibles);
}

void planificador_analizar_mensaje(int32_t socket_r,
		enum tipo_paquete tipoMensaje, char* mensaje, t_niveles_sistema *nivel) {
	char *str_nivel = string_from_format("%d", nivel->nivel);

	switch (tipoMensaje) {

	case PER_nivelFinalizado_PLA: {
		//pasamanos al nivel, lo saco de listos
		t_list *p_listos = dictionary_get(listos, str_nivel);
		int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
			return nuevo->fd == socket_r;
		}
		pthread_mutex_lock(&mutex_listos);
		t_pers_por_nivel *aux = list_remove_by_condition(p_listos,
				(void*) _esta_personaje); //el primer elemento de la lista
		pthread_mutex_unlock(&mutex_listos);
		log_info(logger_pla, "el personaje %c termino este nivel %d",
				aux->personaje, nivel->nivel);
		proceso_desbloqueo(aux->recursos_obtenidos, nivel->fd, str_nivel);
		destruir_personaje(aux);

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

		t_list *p_listos = dictionary_get(listos, str_nivel);

		t_pers_por_nivel *aux;
		int j, i = 0;
		t_list *recursos = list_create();
		while (mensaje[i] != '\0') {

			int32_t _esta_personaje(t_pers_por_nivel *nuevo) {
				return nuevo->personaje == mensaje[i];
			}

			pthread_mutex_lock(&mutex_listos);
			aux = list_remove_by_condition(p_listos, (void*) _esta_personaje);
			pthread_mutex_unlock(&mutex_listos);
			log_info(logger_pla,
					"se saca a %c de la lista de listos por haber muerto interbloqueado",
					aux->personaje);
			j = 0;
			while (!list_is_empty(aux->recursos_obtenidos)) {
				t_recursos_obtenidos* rec = list_remove(aux->recursos_obtenidos,
						j);

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

				j++;
			}
			enviarMensaje(aux->fd, PLA_teMatamos_PER, "0");
			list_destroy(aux->recursos_obtenidos);
			free(aux);
			i++;
		}

		proceso_desbloqueo(recursos, nivel->fd, str_nivel);
		/*t_list *recursosDisponibles = desbloquear_personajes(recursos,
		 str_nivel);
		 char *recursosNuevos = transformarListaCadena(recursosDisponibles);
		 enviarMensaje(nivel->fd, PLA_actualizarRecursos_NIV, recursosNuevos);

		 list_destroy(recursosDisponibles);
		 */
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
		proceso_desbloqueo(aux->recursos_obtenidos, nivel->fd, str_nivel);

		/*t_list *recursosDisponibles = desbloquear_personajes(
		 aux->recursos_obtenidos, str_nivel);

		 char *recursosNuevos = transformarListaCadena(recursosDisponibles);
		 enviarMensaje(nivel->fd, PLA_actualizarRecursos_NIV, recursosNuevos);

		 enviarMensaje(aux->fd, PLA_rtaRecurso_PER, "1");

		 list_destroy(recursosDisponibles);
		 */
		list_destroy(aux->recursos_obtenidos);
		free(aux);
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
	case PER_finPlanDeNiveles_ORQ: {

		char personaje = mensaje[0];
		//estructura: personajes en el sistema
		int32_t _esta_personaje(t_pers_koopa *koopa) {
			return koopa->personaje == personaje;
		}
		pthread_mutex_lock(&mutex_personajes_para_koopa);
		t_pers_koopa *aux = list_find(personajes_para_koopa,
				(void*) _esta_personaje);
		if (aux != NULL )
			aux->termino_plan = true;
		else
			log_info(logger_pla, "error al buscar el personaje");
		//todo: hacer tratamiento de errores
		pthread_mutex_unlock(&mutex_personajes_para_koopa);

		//pregunto si _ya_terminaron terminaron todos
		int32_t _ya_terminaron(t_pers_koopa *koopa) {
			return (koopa->termino_plan);
		}
		/*t_list* pendientes = list_filter(personajes_del_sistema,
		 (void*) _esta_pendiente);
		 if (list_is_empty(pendientes))*/
		//bool list_all_satisfy(t_list* self, bool(*condition)(void*));
		if (list_all_satisfy(personajes_para_koopa, (void*) _ya_terminaron)) {
			log_info(logger_pla, "lanzar_koopa();");

			int32_t pid = fork();
			if (pid == 0) { //si es el hijo
				char * const paramList[] =
						{ ruta_koopa, ruta_disco, ruta_script };
				execv(ruta_koopa, paramList);
				exit(0);
			} else { //si es el padre
				int retorno = 0;
				wait(&retorno);
				log_info(logger_pla,
						"La ejecucion de koopa retorno el valor %d, el pid del proceso era %d",
						retorno, pid);
				if (retorno == pid)
					log_info(logger_pla,
							"Ambos valores son iguales. FIN DEL JUEGO");
				exit(0);
			}
		}
		//list_destroy(pendientes);
		//pregunto si ya terminaron todos
		break;
	}
	default:
		log_info(logger_pla, "mensaje erroneo");
		supr_pers_de_estructuras(socket_r);
		break;
	}

	free(mensaje);
}
