/*
 * orquestador.c
 *
 * Created on: 28/09/2013
 * Author: utnso
 */

#include "orquestador.h"

#define DIRECCION INADDR_ANY //INADDR_ANY representa la direccion de cualquier
//interfaz conectada con la computadora

#define PATH_CONFIG_ORQ "./orq.conf"

t_dictionary *listos;
t_dictionary *bloqueados;
//t_dictionary *anormales;
//t_dictionary *monitoreo;

pthread_mutex_t mutex_listos;
pthread_mutex_t mutex_bloqueados;
//pthread_mutex_t mutex_monitoreo;
//pthread_mutex_t mutex_anormales;
pthread_mutex_t mutex_personajes_para_koopa;
pthread_mutex_t mutex_log;
pthread_mutex_t mutex_personajes_sistema;
//pthread_mutex_t mutex_niveles_sistema;

int32_t puerto;
t_config *config;
t_log* logger;
extern t_log* logger_pla;
t_list *personajes_para_koopa; // es para lanzar koopa
t_list *personajes_del_sistema; // es para identificar los simbolos de los personajes
t_list *niveles_del_sistema;
//int32_t comienzo = 0;
char *ruta_koopa;
char *ruta_script;
char *ruta_disco;

extern void tratamiento_muerte_natural(t_pers_por_nivel *personaje,
		int32_t nivel_fd);

t_pers_koopa *per_koopa_crear(char personaje) {
	t_pers_koopa *nuevo = malloc(sizeof(t_pers_koopa));
	nuevo->personaje = personaje;
	nuevo->termino_plan = false;
	return nuevo;
}

t_pers_por_nivel *crear_personaje(char personaje, int32_t fd) {
	t_pers_por_nivel *nuevo = malloc(sizeof(t_pers_por_nivel));
	nuevo->personaje = personaje;
	nuevo->fd = fd;
	nuevo->pos_inicial = 0;
	nuevo->pos_recurso = 0;
	nuevo->borrame = true;
	nuevo->estoy_bloqueado = false;
	nuevo->recursos_obtenidos = list_create();
	return nuevo;
}

void destruir_personaje(t_pers_por_nivel *personaje) {

	//  int j = 0;
	while (!list_is_empty(personaje->recursos_obtenidos)) {

		t_recursos_obtenidos* rec = list_remove(personaje->recursos_obtenidos,
				0);
		free(rec);
		// j++;
	}
	free(personaje);

}

t_monitoreo *per_monitor_crear(char personaje, int32_t nivel, int32_t socket) {
	t_monitoreo *nuevo = malloc(sizeof(t_monitoreo));
	nuevo->simbolo = personaje;
	nuevo->fd = socket;
	nuevo->nivel = nivel;
	nuevo->monitoreado = false;
	return nuevo;
}

t_list * desbloquear_personajes(t_list *recursos_obtenidos, char *str_nivel,
		int32_t nivel_fd) {

	t_list *recursos_libres = list_create();
	t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);
	t_list *p_listos = dictionary_get(listos, str_nivel);
	char* personajes_desbloqueados = string_new();
	int j = 0;
	bool desbloquie = true;
	if (list_is_empty(p_bloqueados) || (p_bloqueados == NULL )) {
		desbloquie = false;
		log_info(logger,
				"Nivel %s: DesbloquearPersonajes: No habia personajes que desbloquear",
				str_nivel);
		recursos_libres = recursos_obtenidos;
	} else {

		//poner que ademas pregunte si no hay mas personajes para desbloquear
		t_recursos_obtenidos* rec = NULL;
		while (!list_is_empty(recursos_obtenidos)) {

			rec = list_remove(recursos_obtenidos, 0);

			int32_t bloqueadoXrecursos(t_pers_por_nivel *valor) {
				return valor->recurso_bloqueo == rec->recurso;
			}

			pthread_mutex_lock(&mutex_bloqueados);
			t_pers_por_nivel* per = list_remove_by_condition(p_bloqueados,
					(void*) bloqueadoXrecursos);
			pthread_mutex_unlock(&mutex_bloqueados);

			if (per == NULL ) {
				log_info(logger,
						"Nivel %s: DesbloquearPersonajes: No habia personajes que desbloquear con el recurso %c ",
						str_nivel, rec->recurso);
				list_add(recursos_libres, rec);
				//j++;
			} else {
				//desbloquear al personaje! =) y buscar algun otro
				log_info(logger,
						"Nivel %s: DesbloquearPersonajes: se desbloquea a %c y pasa a la lista de listos",
						str_nivel, per->personaje);

				per->estoy_bloqueado = false;
				rec->cantidad--;

				int32_t _esta_recurso(t_recursos_obtenidos *nuevo) {
					return nuevo->recurso == rec->recurso;
				}

				if (list_is_empty(per->recursos_obtenidos)) {

					t_recursos_obtenidos *recu = malloc(
							sizeof(t_recursos_obtenidos));
					recu->recurso = rec->recurso;
					recu->cantidad = 1;
					list_add(per->recursos_obtenidos, recu);
				} else {
					t_recursos_obtenidos *recu = list_find(
							per->recursos_obtenidos, (void*) _esta_recurso);
					if (recu == NULL ) {
						recu = malloc(sizeof(t_recursos_obtenidos));
						recu->recurso = rec->recurso;
						recu->cantidad = 1;
						list_add(per->recursos_obtenidos, recu);
					} else
						recu->cantidad++;
				}

				pthread_mutex_lock(&mutex_log);
				log_info(logger_pla,
						"Nivel %s: Envio al personaje %c que obtuvo su recurso %c",
						str_nivel, per->personaje, rec->recurso);
				pthread_mutex_unlock(&mutex_log);
				enviarMensaje(per->fd, PLA_rtaRecurso_PER, "0");
				pthread_mutex_lock(&mutex_listos);
				list_add(p_listos, per);
				pthread_mutex_unlock(&mutex_listos);
				imprimir_lista(LISTA_BLOQUEADOS, str_nivel);
				imprimir_lista(LISTA_LISTOS, str_nivel);
				//log_info(logger, "sale %c de bloqueados y pasa a la lista de listos ", per->personaje);
				string_append(&personajes_desbloqueados,
						string_from_format("%c", per->personaje));
				string_append(&personajes_desbloqueados, ",");
				j++;
				if (rec->cantidad == 0) {
					free(rec);

				} else {

					list_add(recursos_obtenidos, rec);

				}

			}

		}

	}
	if (personajes_desbloqueados[strlen(personajes_desbloqueados) - 1] == ',')
		personajes_desbloqueados = string_substring_until(
				personajes_desbloqueados, strlen(personajes_desbloqueados) - 1);

	char *cantidad = string_from_format("%d", j);

	string_append(&cantidad, ",");
	string_append(&cantidad, personajes_desbloqueados);

	if (desbloquie || (j != 0) || !string_equals_ignore_case(cantidad, "0,")) {
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %s: Envio al nivel los personajes desbloqueados %s",
				str_nivel, cantidad);
		pthread_mutex_unlock(&mutex_log);

		enviarMensaje(nivel_fd, PLA_personajesDesbloqueados_NIV, cantidad);
	}
	return recursos_libres;
}

void destruir_nivel(t_niveles_sistema * nivel) {

	t_monitoreo * sacado = NULL;
	while (!list_is_empty(nivel->pers_conectados)) {
		sacado = list_remove(nivel->pers_conectados, 0);
		free(sacado);
	}
	while (!list_is_empty(nivel->pers_desconectados)) {
		sacado = list_remove(nivel->pers_desconectados, 0);
		free(sacado);
	}

	log_info(logger, "Se cancela el hilo planificador del nivel %s",
			nivel->str_nivel);
	pthread_cancel(nivel->pla);	//, SIGTERM);
	free(nivel);
}
void suprimir_de_estructuras(int32_t sockett, t_pers_por_nivel* personaje) {
	//si se desconecta un nivel, automaticamente se caen los personajes que estaban conectados
	//si se cae un personaje, el mensaje le llega al planificador directamente

	int32_t _esta_enSistema(t_monitoreo *valor) {
		return valor->fd == sockett;
	}
	int32_t _esta_enNiveles(t_niveles_sistema *valor) {
		return valor->fd == sockett;
	}
	pthread_mutex_lock(&mutex_personajes_sistema);
	t_monitoreo *p_sistema = list_remove_by_condition(personajes_del_sistema,
			(void*) _esta_enSistema);
	pthread_mutex_unlock(&mutex_personajes_sistema);

	//pthread_mutex_lock(&mutex_niveles_sistema);
	t_niveles_sistema * n_sistema = list_remove_by_condition(
			niveles_del_sistema, (void*) _esta_enNiveles);
	//pthread_mutex_unlock(&mutex_niveles_sistema);

	if ((p_sistema == NULL )&& (n_sistema == NULL))return;

	char *str_nivel = NULL;

	if (p_sistema == NULL ) {

		//se cayo un nivel
		str_nivel = n_sistema->str_nivel;
		log_info(logger, "Se cayo el nivel %s.", str_nivel);

		t_list *l_listos = dictionary_remove(listos, str_nivel);
		t_list *l_bloqueados = dictionary_remove(bloqueados, str_nivel);
		//t_list *l_anormales = dictionary_remove(anormales, str_nivel);
		//t_list *l_monitoreo = dictionary_remove(monitoreo, str_nivel);

		t_pers_por_nivel *elemento_personaje = personaje;
		t_monitoreo *elemento_monitoreo = NULL;
		//	int32_t *anormal = NULL;

		if (elemento_personaje != NULL ) {//se estaba planificando o pululaba por ahi

			int32_t _esta_enSistema2(t_monitoreo *valor) {
				return valor->fd == personaje->fd;
			}

			pthread_mutex_lock(&mutex_personajes_sistema);
			elemento_monitoreo = list_remove_by_condition(
					personajes_del_sistema, (void*) _esta_enSistema2);
			pthread_mutex_unlock(&mutex_personajes_sistema);

			if (elemento_monitoreo != NULL ) {
				log_info(logger,
						"Le aviso al personaje %c que el nivel %s se cayo",
						elemento_personaje->personaje, str_nivel);
				plan_enviarMensaje(n_sistema, elemento_personaje->fd,
						PLA_nivelCaido_PER, str_nivel);
				//close(elemento_personaje->fd);
				elemento_monitoreo = NULL;
			}

		}

		pthread_mutex_lock(&mutex_listos);
		while (!list_is_empty(l_listos) && (l_listos != NULL )) {
			elemento_personaje = list_remove(l_listos, 0);
			destruir_personaje(elemento_personaje);
		}
		list_destroy(l_listos);
		pthread_mutex_unlock(&mutex_listos);

		pthread_mutex_lock(&mutex_bloqueados);
		while (!list_is_empty(l_bloqueados) && (l_bloqueados != NULL )) {
			elemento_personaje = list_remove(l_bloqueados, 0);
			destruir_personaje(elemento_personaje);
		}
		list_destroy(l_bloqueados);
		pthread_mutex_unlock(&mutex_bloqueados);

		destruir_nivel(n_sistema);

	} else {
		//era un personaje lo que se desconecto

		if (p_sistema->nivel != 0) {

			int32_t _esta_enListas(t_pers_por_nivel *valor) {
				return valor->fd == sockett;
			}

			int32_t _buscar_nivel(t_niveles_sistema *valor) {
				return valor->nivel == p_sistema->nivel;
			}

			t_niveles_sistema * nivel = list_find(niveles_del_sistema,
					(void *) _buscar_nivel);

			str_nivel = nivel->str_nivel;

			t_list *p_listos = dictionary_get(listos, str_nivel);
			t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);

			t_pers_por_nivel * p_listas = personaje;

			if((p_listas == NULL)||(p_listas->personaje != p_listas->personaje)){
				//o sea que se murio el que se estaba planificando entonces no va a estar en listos ni bloqueados

			if (p_bloqueados != NULL ) {
				pthread_mutex_lock(&mutex_bloqueados);
				p_listas = list_remove_by_condition(p_bloqueados,
						(void*) _esta_enListas);
				pthread_mutex_unlock(&mutex_bloqueados);
			}

			if (p_listas == NULL ) {
				if (p_listos != NULL ) {
					pthread_mutex_lock(&mutex_listos);
					p_listas = list_remove_by_condition(p_listos,
							(void*) _esta_enListas);
					pthread_mutex_unlock(&mutex_listos);
				}
			}
			}
			if (p_listas != NULL ) {
				pthread_mutex_lock(&mutex_log);
				log_info(logger_pla,
						"Nivel %s: El personaje %c del soket %d se desconecto ",
						str_nivel, p_listas->personaje, p_listas->fd);
				log_info(logger_pla,
						"Nivel %d: Le aviso al nivel que el personaje %c murio por causas externas",
						nivel->nivel, p_listas->personaje);
				pthread_mutex_unlock(&mutex_log);
				char* muerto = string_from_format("%c", p_listas->personaje);
				enviarMensaje(nivel->fd, PLA_perMuereNaturalmente_NIV, muerto);

				if (!list_is_empty(p_listas->recursos_obtenidos)) {
					proceso_desbloqueo(p_listas->recursos_obtenidos, nivel->fd,
							str_nivel);
				}
				destruir_personaje(p_listas);

			}


			//lo saco de normales y lo pongo en muertos
			t_monitoreo *aux1 = NULL;
			pthread_mutex_lock(&(nivel->mutex_lista));
			aux1 = list_remove_by_condition(nivel->pers_conectados,
					(void*) _esta_enSistema);
			if (aux1 != NULL ) {
				list_add(nivel->pers_desconectados, aux1);
				nivel->cant_pers--;
			}
			pthread_mutex_unlock(&(nivel->mutex_lista));
			//lo saco de normales y lo pongo en muertos

		}
	}
	free(p_sistema);
}

void suprimir_personaje_de_estructuras(t_pers_por_nivel* personaje) {

	int32_t _esta_enSistema(t_monitoreo *valor) {
		return valor->fd == personaje->fd;
	}
	int32_t _esta_enListas(t_pers_por_nivel *valor) {
		return valor->fd == personaje->fd;
	}
	pthread_mutex_lock(&mutex_personajes_sistema);
	t_monitoreo *aux = list_remove_by_condition(personajes_del_sistema,
			(void*) _esta_enSistema);
	pthread_mutex_unlock(&mutex_personajes_sistema);
	char *str_nivel;

	if (aux == NULL ) {
		log_info(logger,
				"suprimir_personaje_de_estructuras: el personaje ya fue borrado");

	} else {

		if (aux->nivel != 0) { //o sea que no se desconecto apenas entro.. aca nunca va a pasar

			int32_t _buscar_nivel(t_niveles_sistema *valor) {
				return valor->nivel == aux->nivel;
			}
			t_niveles_sistema * nivel = list_find(niveles_del_sistema,
					(void *) _buscar_nivel);

			str_nivel = nivel->str_nivel;

			//lo saco de normales y lo pongo en muertos
			//pthread_mutex_lock(&mutex_niveles_sistema);
			t_monitoreo *aux1 = NULL;
			pthread_mutex_lock(&(nivel->mutex_lista));
			aux1 = list_remove_by_condition(nivel->pers_conectados,
					(void*) _esta_enSistema);
			if (aux1 != NULL ) {
				list_add(nivel->pers_desconectados, aux1);
				nivel->cant_pers--;
			/*	if (nivel->cant_pers == 0) {
					pthread_mutex_lock(&(nivel->mutex_inicial));
				}
			*/
			}
			pthread_mutex_unlock(&(nivel->mutex_lista));
			//pthread_mutex_unlock(&mutex_niveles_sistema);
			//lo saco de normales y lo pongo en muertos

			pthread_mutex_lock(&mutex_log);
			log_info(logger_pla,
					"Nivel %s: El personaje %c del socket %d se desconecto ",
					str_nivel, personaje->personaje, personaje->fd);
			pthread_mutex_unlock(&mutex_log);

			if (!list_is_empty(personaje->recursos_obtenidos)) {
				proceso_desbloqueo(personaje->recursos_obtenidos, nivel->fd,
						str_nivel);
			}
			//agregar_anormales(str_nivel, personaje->fd);
			//close(personaje->fd);
			destruir_personaje(personaje);

		}
	}
	free(aux);
}

void imprimir_lista(int32_t tipo, char *str_nivel) {

	t_pers_por_nivel *elemento = NULL;
	int i;
	char * cadena = NULL;
	if (tipo == LISTA_LISTOS) {
		//lista de listos
		t_list *p_listos = dictionary_get(listos, str_nivel);
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %s: Se imprime lista de personajes listos *************",
				str_nivel);
		pthread_mutex_unlock(&mutex_log);
		pthread_mutex_lock(&mutex_listos);
		for (i = 0; i < list_size(p_listos); i++) {
			elemento = list_get(p_listos, i);
			t_list *auxiliar = list_create();
			list_add_all(auxiliar, elemento->recursos_obtenidos);
			cadena = transformarListaCadena(auxiliar);
			list_destroy(auxiliar);
			pthread_mutex_lock(&mutex_log);
			log_info(logger_pla,
					"Nivel %s: Personaje: %c, posee los recursos: %s, remaining distance: %d",
					str_nivel, elemento->personaje, cadena,
					abs(elemento->pos_recurso - elemento->pos_inicial));
			pthread_mutex_unlock(&mutex_log);
			free(cadena);
		}

		pthread_mutex_unlock(&mutex_listos);
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %s: Fin de impresión de la lista de personajes listos *************",
				str_nivel);
		pthread_mutex_unlock(&mutex_log);
	} else {
		// lista de bloqueados
		t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %s: Se imprime lista de personajes bloqueados -------------",
				str_nivel);
		pthread_mutex_unlock(&mutex_log);
		pthread_mutex_lock(&mutex_bloqueados);
		for (i = 0; i < list_size(p_bloqueados); i++) {
			elemento = list_get(p_bloqueados, i);
			t_list *auxiliar = list_create();
			list_add_all(auxiliar, elemento->recursos_obtenidos);
			cadena = transformarListaCadena(auxiliar);
			list_destroy(auxiliar);
			int remain = abs(elemento->pos_recurso - elemento->pos_inicial);
			pthread_mutex_lock(&mutex_log);
			log_info(logger_pla,
					"Nivel %s: Personaje: %c, bloqueado por: %c, posee los recursos: %s, remaining distance: %d",
					str_nivel, elemento->personaje, elemento->recurso_bloqueo,
					cadena, remain);
			pthread_mutex_unlock(&mutex_log);
			free(cadena);
		}
		pthread_mutex_unlock(&mutex_bloqueados);
		pthread_mutex_lock(&mutex_log);
		log_info(logger_pla,
				"Nivel %s: Fin de impresión de la lista de personajes bloqueados -------------",
				str_nivel);
		pthread_mutex_unlock(&mutex_log);
	}

}

void orquestador_analizar_mensaje(int32_t socket, enum tipo_paquete tipoMensaje,
		char* mensaje);

int main() {

	//inicialización
	config = config_create(PATH_CONFIG_ORQ);

	char *PATH_LOG = config_get_string_value(config, "PATH_LOG_ORQ");
	logger = log_create(PATH_LOG, "ORQUESTADOR", true, LOG_LEVEL_INFO);
	puerto = config_get_int_value(config, "PUERTO");
	char* auxiliar = config_get_string_value(config, "koopa");
	ruta_koopa = calloc(strlen(auxiliar) + 1, sizeof(char));
	strcpy(ruta_koopa, auxiliar);
	// free(auxiliar);
	auxiliar = config_get_string_value(config, "script");
	ruta_script = calloc(strlen(auxiliar) + 1, sizeof(char));
	strcpy(ruta_script, auxiliar);
	//free(auxiliar);
	auxiliar = config_get_string_value(config, "mapeo");
	ruta_disco = calloc(strlen(auxiliar) + 1, sizeof(char));
	strcpy(ruta_disco, auxiliar);

	config_destroy(config);

	personajes_del_sistema = list_create();
	niveles_del_sistema = list_create();
	personajes_para_koopa = list_create();
	listos = dictionary_create();
	bloqueados = dictionary_create();
	//anormales = dictionary_create();
	//monitoreo = dictionary_create();
	pthread_mutex_init(&mutex_listos, NULL );
	pthread_mutex_init(&mutex_bloqueados, NULL );
	//pthread_mutex_init(&mutex_monitoreo, NULL );
	pthread_mutex_init(&mutex_log, NULL );
	//pthread_mutex_init(&mutex_anormales, NULL );
	pthread_mutex_init(&mutex_personajes_para_koopa, NULL );
	pthread_mutex_init(&mutex_personajes_sistema, NULL );
	//pthread_mutex_init(&mutex_niveles_sistema, NULL );
	//inicialización

	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	struct sockaddr_in remoteaddr; // dirección del cliente
	int fdmax; // número máximo de descriptores de fichero
	int32_t socketEscucha; // descriptor de socket a la escucha
	int newfd; // descriptor de socket de nueva conexión aceptada
	socklen_t addrlen;
	int i;
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);

	socketEscucha = crearSocketDeConexion(DIRECCION, puerto);

	// añadir socketEscucha al conjunto maestro
	FD_SET(socketEscucha, &master);

	// seguir la pista del descriptor de fichero mayor
	fdmax = socketEscucha; // por ahora es éste porque es el primero y unico

	// bucle principal
	log_info(logger, "Empieza el juego!!");
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
						log_info(logger, "Nueva conexion de %s en el socket %d",
								inet_ntoa(remoteaddr.sin_addr), newfd);
					}
				} else {

					enum tipo_paquete tipoMensaje = PER_posCajaRecurso_PLA;
					char* mensaje = NULL;

					// gestionar datos del cliente del socket i!
					if (recibirMensaje(i, &tipoMensaje,
							&mensaje)!= EXIT_SUCCESS) {

						//eliminarlo de las estructuras
						t_pers_por_nivel* v = NULL;
						suprimir_de_estructuras(i, v);
						//eliminarlo de las estructuras

						close(i); // ¡Hasta luego!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					} else {
						// tenemos datos del cliente del socket i!
						log_info(logger, "Llego el tipo de paquete: %s .",
								obtenerNombreEnum(tipoMensaje));
						log_info(logger, "Llego este mensaje: %s .", mensaje);

						if (tipoMensaje == PER_conexionNivel_ORQ) {
//								|| (tipoMensaje == NIV_handshake_ORQ))

							t_list *p_listos = dictionary_get(listos, mensaje);
							if (p_listos != NULL ) {
								FD_CLR(i, &master); // eliminar del conjunto maestro
							}

						} else {
							if (tipoMensaje == NIV_handshake_ORQ) {
								FD_CLR(i, &master); // eliminar del conjunto maestro
							}
						}
						orquestador_analizar_mensaje(i, tipoMensaje, mensaje);

					} // fin seccion recibir OK los datos
				} // fin gestionar datos que llegaron
			} // fin de tenemos datos
		} // fin exploración de datos nuevos
	} // fin bucle for principal

	return EXIT_SUCCESS;
}

void orquestador_analizar_mensaje(int32_t sockett,
		enum tipo_paquete tipoMensaje, char* mensaje) {
	switch (tipoMensaje) {

	case NIV_handshake_ORQ: {
		//mensaje = nivel,algoritmo,quantum,retardo,remain_distance = "1,RR,4,1000,6"

		char** n_mensaje = string_split(mensaje, ",");
		int32_t nivel = atoi(n_mensaje[0]);

//                recu = string_from_format("%c", mensaje[0]);

		t_list *p_listos = list_create();
		t_list *p_bloqueados = list_create();
		//t_list *p_muertos = list_create();
		//t_list *p_monitor = list_create();

		dictionary_put(listos, n_mensaje[0], p_listos);
		dictionary_put(bloqueados, n_mensaje[0], p_bloqueados);
		//dictionary_put(anormales, n_mensaje[0], p_muertos);
		//dictionary_put(monitoreo, n_mensaje[0], p_monitor);

		t_niveles_sistema *nuevo = malloc(sizeof(t_niveles_sistema));
		nuevo->nivel = nivel;
		nuevo->str_nivel = string_new();
		string_append(&(nuevo->str_nivel), n_mensaje[0]);
		nuevo->fd = sockett;
		nuevo->algol = n_mensaje[1];
		nuevo->quantum = atoi(n_mensaje[2]);
		nuevo->retardo = atoi(n_mensaje[3]);
		nuevo->remain_distance = atoi(n_mensaje[4]);
		nuevo->pers_conectados = list_create();
		nuevo->cant_pers = 0;
		nuevo->pers_desconectados = list_create();
		pthread_mutex_init(&(nuevo->mutex_inicial), NULL );
		pthread_mutex_init(&(nuevo->mutex_lista), NULL );

		//lo bloqueo porque todavia no hay nadie
		pthread_mutex_lock(&(nuevo->mutex_inicial));

		//pthread_mutex_lock(&mutex_niveles_sistema);
		list_add(niveles_del_sistema, nuevo);
		//pthread_mutex_unlock(&mutex_niveles_sistema);

		/*		//DELEGAR TMB EL SOCKET AL PLANIFICADOR:
		 // delegar la conexión al hilo del nivel correspondiente
		 t_list *pe_monitor = dictionary_get(monitoreo, n_mensaje[0]);
		 t_monitoreo *item = per_monitor_crear(false, 'M', atoi(n_mensaje[0]),
		 sockett);
		 pthread_mutex_lock(&mutex_monitoreo);
		 list_add(pe_monitor, item);
		 pthread_mutex_unlock(&mutex_monitoreo);
		 // delegar la conexión al hilo del nivel correspondiente
		 */
		log_info(logger,
				"Se crea el hilo planificador para el nivel (%d) recien conectado con fd= %d ",
				nivel, sockett);

		pthread_create(&nuevo->pla, NULL, (void *) hilo_planificador, nuevo);

		log_info(logger, "Envío respuesta handshake al nivel (%d).", nivel);
		enviarMensaje(sockett, ORQ_handshake_NIV, "0");

		break;
	}
	case PER_conexionNivel_ORQ: {
		int32_t nivel = atoi(mensaje);

		int32_t _esta_personaje(t_monitoreo *nuevo) {
			return nuevo->fd == sockett;
		}

		pthread_mutex_lock(&mutex_personajes_sistema);

		t_monitoreo *aux = list_find(personajes_del_sistema,
				(void*) _esta_personaje);
		// delegar la conexión al hilo del nivel correspondiente
		log_info(logger, "Nivel solicitado por el personaje %c: %d ",
				aux->simbolo, nivel);

		t_list *p_listos = dictionary_get(listos, mensaje);
		if (p_listos == NULL ) {
			log_info(logger,
					"El nivel solicitado: %d aun no se encuentra disponible en el sistema, por favor intente mas tarde.",
					nivel);

			enviarMensaje(sockett, ORQ_conexionNivel_PER, "1");
		} else {

			aux->nivel = nivel; //actualiza el nivel a donde se conecta el personaje
			/*t_list *p_anormales = dictionary_get(anormales, mensaje);
			 pthread_mutex_lock(&mutex_anormales);

			 int32_t _esta_fd(int32_t *nuevo) {
			 return *nuevo != sockett;
			 }
			 //me saca los que sean distintos
			 t_list* caso_ejemplo = list_filter(p_anormales, (void*) _esta_fd);
			 list_clean(p_anormales);
			 list_add_all(p_anormales, caso_ejemplo);
			 pthread_mutex_unlock(&mutex_anormales);
			 */

			t_monitoreo *item = per_monitor_crear(aux->simbolo, nivel, sockett);
			memcpy(item, aux, sizeof(t_monitoreo));

			// delegar la conexión al hilo del nivel correspondiente
			int32_t _esta_nivel(t_niveles_sistema *nuevo) {
				return nuevo->nivel == nivel;
			}


			t_niveles_sistema *niv = list_find(niveles_del_sistema,
					(void*) _esta_nivel);
			pthread_mutex_lock(&(niv->mutex_lista));
			//pthread_mutex_lock(&mutex_niveles_sistema);
			list_add(niv->pers_conectados, item);
			niv->cant_pers++;
			//pthread_mutex_unlock(&mutex_niveles_sistema);
			pthread_mutex_unlock(&(niv->mutex_lista));

			// delegar la conexión al hilo del nivel correspondiente

			//agregar a la lista de listos del nivel
			t_list *p_listos = dictionary_get(listos, mensaje);
			t_pers_por_nivel *item2 = crear_personaje(aux->simbolo, aux->fd);

			item2->pos_recurso = niv->remain_distance;
			pthread_mutex_lock(&mutex_listos);
			list_add(p_listos, item2);
			pthread_mutex_unlock(&mutex_listos);
			//agregar a la lista de listos del nivel
			enviarMensaje(sockett, ORQ_conexionNivel_PER, "0");

			//desbloquea al nivel para que planifique todo: probar bien esto
			if (niv->cant_pers == 1) {
				//si es 1 es por el que recién agregue
				pthread_mutex_unlock(&(niv->mutex_inicial));
			}

		}
		pthread_mutex_unlock(&mutex_personajes_sistema);
		break;
	}
	case PER_handshake_ORQ: {

		char personaje = mensaje[0];

		//estructura: personajes_para_koopa
		int32_t _esta_personaje(t_pers_koopa *koopa) {
			return koopa->personaje == personaje;
		}
		if (list_is_empty(personajes_para_koopa)) {
			t_pers_koopa * item = per_koopa_crear(personaje);
			pthread_mutex_lock(&mutex_personajes_para_koopa);
			list_add(personajes_para_koopa, item);
			pthread_mutex_unlock(&mutex_personajes_para_koopa);
		} else {
			if (list_find(personajes_para_koopa,
					(void*) _esta_personaje) == NULL) {
				t_pers_koopa * item = per_koopa_crear(personaje);
				pthread_mutex_lock(&mutex_personajes_para_koopa);
				list_add(personajes_para_koopa, item);
				pthread_mutex_unlock(&mutex_personajes_para_koopa);
			}
		}
		//estructura: personajes_para_koopa

		//estructura: personajes_del_sistema
		t_monitoreo *item = per_monitor_crear(personaje, 0, sockett);
		pthread_mutex_lock(&mutex_personajes_sistema);
		list_add(personajes_del_sistema, item);
		pthread_mutex_unlock(&mutex_personajes_sistema);
		//estructura: personajes_del_sistema
		log_info(logger, "Envío respuesta handshake al personaje (%c).",
				personaje);
		enviarMensaje(sockett, ORQ_handshake_PER, "0");

		break;
	}
	case PER_finPlanDeNiveles_ORQ: {

		char personaje = mensaje[0];
		//estructura: personajes_del_sistema
		t_monitoreo *item = per_monitor_crear(personaje, 0, sockett);
		pthread_mutex_lock(&mutex_personajes_sistema);
		list_add(personajes_del_sistema, item);
		pthread_mutex_unlock(&mutex_personajes_sistema);
		//estructura: personajes_del_sistema

		int32_t _esta_personaje(t_pers_koopa *koopa) {
			return koopa->personaje == personaje;
		}
		pthread_mutex_lock(&mutex_personajes_para_koopa);
		t_pers_koopa *aux = list_find(personajes_para_koopa,
				(void*) _esta_personaje);
		if (aux != NULL )
			aux->termino_plan = true;
		else
			log_info(logger, "error al buscar el personaje");
		// hacer tratamiento de errores
		pthread_mutex_unlock(&mutex_personajes_para_koopa);

		//pregunto si _ya_terminaron terminaron todos
		int32_t _ya_terminaron(t_pers_koopa *koopa) {
			return (koopa->termino_plan);
		}

		if (list_all_satisfy(personajes_para_koopa, (void*) _ya_terminaron)) {
			log_info(logger, "Se lanza koopa =)");

			while(!list_is_empty(niveles_del_sistema)){
				t_niveles_sistema *ni = list_remove(niveles_del_sistema,0);
				close(ni->fd);
				free(ni);

			}
			log_info(logger, "Sali de while =)");

			int32_t pid = fork();
			if (pid == 0) { //si es el hijo
				log_info(logger, "Fork Koopa =)");
				char * const paramList[] =
						{ ruta_koopa, ruta_disco, ruta_script,NULL };
				execv(ruta_koopa, paramList);
				//exit(0);
			} else { //si es el padre
				int retorno = 0;
				wait(&retorno);
				log_info(logger,
						"La ejecucion de koopa retorno el valor %d.",
						retorno);
				//if (retorno == pid)
				log_info(logger, "FIN DEL JUEGO");
				exit(0);
			}
		}

		//pregunto si ya terminaron todos
		break;
	}

	default:
		log_info(logger, "mensaje erroneo");
		break;

	}

	free(mensaje);

}
