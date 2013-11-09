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
t_dictionary *monitoreo;

pthread_mutex_t mutex_listos;
pthread_mutex_t mutex_bloqueados;
pthread_mutex_t mutex_monitoreo;
pthread_mutex_t mutex_personajes_para_koopa;

int32_t puerto;
t_config *config;
t_log* logger;
t_list *personajes_para_koopa; // es para lanzar koopa
t_list *personajes_del_sistema; // es para identificar los simbolos de los personajes
t_list *niveles_del_sistema;
//int32_t comienzo = 0;

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
	//comienzo++;
	nuevo->estoy_bloqueado = false;
	nuevo->recursos_obtenidos = list_create();
	return nuevo;
}

void destruir_personaje(t_pers_por_nivel *personaje) {

	int j = 0;
	while (!list_is_empty(personaje->recursos_obtenidos)) {

		t_recursos_obtenidos* rec = list_remove(personaje->recursos_obtenidos,
				j);
		free(rec);
		j++;
	}
	free(personaje);

}

t_monitoreo *per_monitor_crear(bool es_personaje, char personaje, int32_t nivel,
		int32_t socket) {
	t_monitoreo *nuevo = malloc(sizeof(t_monitoreo));
	nuevo->simbolo = personaje;
	nuevo->fd = socket;
	nuevo->nivel = nivel;
	nuevo->es_personaje = es_personaje;
	return nuevo;
}

t_list * desbloquear_personajes(t_list *recursos_obtenidos, char *str_nivel) {

	t_list *recursos_libres = list_create();
	t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);
	t_list *p_listos = dictionary_get(listos, str_nivel);

	if (list_is_empty(p_bloqueados) || (p_bloqueados == NULL )) {
		log_info(logger,
				"DesbloquearPersonajes: No habia personajes que desbloquear");
	} else {
		int j = 0;
		while (!list_is_empty(recursos_obtenidos)) {

			t_recursos_obtenidos* rec = list_remove(recursos_obtenidos, j);

			int32_t bloqueadoXrecursos(t_pers_por_nivel *valor) {
				return valor->recurso_bloqueo == rec->recurso;
			}

			pthread_mutex_lock(&mutex_bloqueados);
			t_pers_por_nivel* per = list_remove_by_condition(p_bloqueados,
					(void*) bloqueadoXrecursos);
			pthread_mutex_unlock(&mutex_bloqueados);

			if (per == NULL ) {
				log_info(logger,
						"DesbloquearPersonajes: No habia personajes que desbloquear con el recurso %c ",
						rec->recurso);
				list_add(recursos_libres, rec);
			} else {
				//desbloquear al personaje! =) y buscar algun otro
				log_info(logger, "DesbloquearPersonajes: se desbloquea a %c ",
						per->personaje);
				per->estoy_bloqueado = false;
				rec->cantidad--;
				pthread_mutex_lock(&mutex_listos);
				list_add(p_listos, per);
				pthread_mutex_unlock(&mutex_listos);

				log_info(logger,
						"sale %c de bloqueados y pasa a la lista de listos ",
						per->personaje);

				if (rec->cantidad == 0) {
					free(rec);
					break;
				} else {
					list_add(recursos_obtenidos, rec);
					j--; //fixme probar bien esto!
				}

			}
			j++;

		}

	}
	return recursos_libres;
}

void supr_pers_de_estructuras(int32_t sockett) {
	//si se desconecta un nivel, automaticamente se caen los personajes que estaban conectados
	//si se cae un personaje, el mensaje le llega al planificador directamente

	int32_t _esta_enSistema(t_monitoreo *valor) {
		return valor->fd == sockett;
	}
	int32_t _esta_enListas(t_pers_por_nivel *valor) {
		return valor->fd == sockett;
	}
	t_monitoreo *aux = list_remove_by_condition(personajes_del_sistema,
			(void*) _esta_enSistema);
	char *str_nivel;

	if (aux == NULL ) {
		/*//se cayo un nivel
		 int32_t _esta_enNiveles(t_niveles_sistema *valor) {
		 return valor->fd == sockett;
		 }
		 t_niveles_sistema * aux3 = list_remove_by_condition(niveles_del_sistema,
		 (void*) _esta_enNiveles);

		 str_nivel = string_from_format("%d", aux3->nivel);

		 t_list *l_listos = dictionary_remove(listos, str_nivel);
		 t_list *l_bloqueados = dictionary_remove(bloqueados, str_nivel);
		 t_list *l_anormales = dictionary_remove(anormales, str_nivel);
		 t_list *l_monitoreo = dictionary_remove(monitoreo, str_nivel);

		 int m;
		 t_pers_por_nivel *elemento = NULL;
		 t_monitoreo *elem = NULL;
		 for (m = 0; !list_is_empty(l_listos); m++) {
		 elemento = list_remove(l_listos, m);
		 free(elemento);
		 }
		 list_destroy(l_listos);
		 for (m = 0; !list_is_empty(l_bloqueados); m++) {
		 elemento = list_remove(l_bloqueados, m);
		 free(elemento);
		 }
		 list_destroy(l_bloqueados);
		 for (m = 0; !list_is_empty(l_anormales); m++) {
		 elemento = list_remove(l_anormales, m);
		 free(elemento);
		 }
		 list_destroy(l_anormales);
		 for (m = 0; !list_is_empty(l_monitoreo); m++) {
		 elem = list_remove(l_monitoreo, m);
		 free(elem);
		 }
		 list_destroy(l_monitoreo);

		 free(aux3);
		 */
	} else {
		//era un personaje lo que se desconecto
		int32_t _esta_enKoopa(t_pers_koopa *valor) {
			return valor->personaje == aux->simbolo;
		}

		pthread_mutex_lock(&mutex_personajes_para_koopa);
		t_monitoreo *aux1 = list_remove_by_condition(personajes_para_koopa,
				(void*) _esta_enKoopa);
		pthread_mutex_unlock(&mutex_personajes_para_koopa);
		free(aux1);

		int32_t _buscar_nivel(t_niveles_sistema *valor) {
			return valor->nivel == aux->nivel;
		}
		t_niveles_sistema * nivel_es = list_find(niveles_del_sistema,
				(void *) _buscar_nivel);

		str_nivel = string_from_format("%d", aux->nivel);
		t_list *p_listos = dictionary_get(listos, str_nivel);
		t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);
		//t_list *p_anormales = dictionary_get(anormales, str_nivel);
		t_list *p_monitoreo = dictionary_get(monitoreo, str_nivel);

		pthread_mutex_lock(&mutex_monitoreo);
		aux1 = list_remove_by_condition(p_monitoreo, (void*) _esta_enSistema);
		pthread_mutex_unlock(&mutex_monitoreo);
		free(aux1);

		pthread_mutex_lock(&mutex_bloqueados);
		t_pers_por_nivel * aux2 = list_remove_by_condition(p_bloqueados,
				(void*) _esta_enListas);
		pthread_mutex_unlock(&mutex_bloqueados);
		if (aux2 == NULL ) {
			//si no estaba en bloqueados

			//        aux2 = list_remove_by_condition(p_anormales,
			//                        (void*) _esta_enListas);

			//if (aux2 == NULL ) {
			//si no estaba en anormales
			pthread_mutex_lock(&mutex_listos);
			aux2 = list_remove_by_condition(p_listos, (void*) _esta_enListas);
			pthread_mutex_unlock(&mutex_listos);
			if (aux2 == NULL ) {
				//si no estaba en listos, se estaba planificando en el momento.. lolaa
				//fixme no hay tratamiento para eso
				log_info(logger,
						"personaje muerto se estaba planificando. Rompe todo!!!");
			}
			//}
		}
		if (aux2 != NULL ) {
			proceso_desbloqueo(aux2->recursos_obtenidos, nivel_es->fd,
					str_nivel);
			free(aux2);
		}
	}
	free(aux);
}

void orquestador_analizar_mensaje(int32_t socket, enum tipo_paquete tipoMensaje,
		char* mensaje);

int main() {

	//inicialización
	config = config_create(PATH_CONFIG_ORQ);
	char *PATH_LOG = config_get_string_value(config, "PATH_LOG_ORQ");
	logger = log_create(PATH_LOG, "ORQUESTADOR", true, LOG_LEVEL_INFO);
	puerto = config_get_int_value(config, "PUERTO");
	personajes_del_sistema = list_create();
	niveles_del_sistema = list_create();
	personajes_para_koopa = list_create();
	listos = dictionary_create();
	bloqueados = dictionary_create();
	//anormales = dictionary_create();
	monitoreo = dictionary_create();
	pthread_mutex_init(&mutex_listos, NULL );
	pthread_mutex_init(&mutex_bloqueados, NULL );
	pthread_mutex_init(&mutex_monitoreo, NULL );
	pthread_mutex_init(&mutex_personajes_para_koopa, NULL );
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
	log_info(logger, "entrando al for!! ");
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
						log_info(logger,
								"selectserver: new connection from %s on "
										"socket %d",
								inet_ntoa(remoteaddr.sin_addr), newfd);
					}
				} else {

					enum tipo_paquete tipoMensaje;
					char* mensaje = NULL;

					// gestionar datos del cliente del socket i!
					if (recibirMensaje(i, &tipoMensaje,
							&mensaje)!= EXIT_SUCCESS) {

						//eliminarlo de las estructuras
						supr_pers_de_estructuras(i);
						//eliminarlo de las estructuras

						close(i); // ¡Hasta luego!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					} else {
						// tenemos datos del cliente del socket i!
						log_info(logger, "Llego el tipo de paquete: %s .",
								obtenerNombreEnum(tipoMensaje));
						log_info(logger, "Llego este mensaje: %s .", mensaje);

						if ((tipoMensaje == PER_conexionNivel_ORQ)
								|| (tipoMensaje == NIV_handshake_ORQ))
							FD_CLR(i, &master); // eliminar del conjunto maestro

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
		//mensaje = nivel,algoritmo,quantum,retardo = "1,RR,4,1000"

		char** n_mensaje = string_split(mensaje, ",");
		int32_t nivel = atoi(n_mensaje[0]);

//		recu = string_from_format("%c", mensaje[0]);

		t_list *p_listos = list_create();
		t_list *p_bloqueados = list_create();
		//t_list *p_muertos = list_create();
		t_list *p_monitor = list_create();

		dictionary_put(listos, n_mensaje[0], p_listos);
		dictionary_put(bloqueados, n_mensaje[0], p_bloqueados);
		//dictionary_put(anormales, n_mensaje[0], p_muertos);
		dictionary_put(monitoreo, n_mensaje[0], p_monitor);

		t_niveles_sistema *nuevo = malloc(sizeof(t_niveles_sistema));
		nuevo->nivel = nivel;
		nuevo->fd = sockett;
		nuevo->algol = n_mensaje[1];
		nuevo->quantum = atoi(n_mensaje[2]);
		nuevo->retardo = atoi(n_mensaje[3]);

		list_add(niveles_del_sistema, nuevo);

		log_info(logger,
				"Se crea el hilo planificador para el nivel (%d) recien conectado con fd= %d ",
				nivel, sockett);
		pthread_t pla;
		pthread_create(&pla, NULL, (void *) hilo_planificador, nuevo);

		//DELEGAR TMB EL SOCKET AL PLANIFICADOR:
		// delegar la conexión al hilo del nivel correspondiente
		t_list *pe_monitor = dictionary_get(monitoreo, mensaje);
		t_monitoreo *item = per_monitor_crear(false, 'M', atoi(n_mensaje[0]),
				sockett);
		pthread_mutex_lock(&mutex_monitoreo);
		list_add(pe_monitor, item);
		pthread_mutex_unlock(&mutex_monitoreo);
		// delegar la conexión al hilo del nivel correspondiente

		enviarMensaje(sockett, ORQ_handshake_NIV, "0");

		break;
	}
	case PER_conexionNivel_ORQ: {
		int32_t nivel = atoi(mensaje);

		int32_t _esta_personaje(t_monitoreo *nuevo) {
			return nuevo->fd == sockett;
		}

		// delegar la conexión al hilo del nivel correspondiente
		log_info(logger, "nivel solicitado: %d ", nivel);
		t_monitoreo *aux = list_find(personajes_del_sistema,
				(void*) _esta_personaje);
		aux->nivel = nivel; //actualiza el nivel a donde se conecta el personaje
		t_list *p_monitor = dictionary_get(monitoreo, mensaje);
		t_monitoreo *item = per_monitor_crear(true, 'M', nivel, sockett);
		memcpy(item, aux, sizeof(t_monitoreo)); //fixme ver bien esto
		pthread_mutex_lock(&mutex_monitoreo);
		list_add(p_monitor, item);
		pthread_mutex_unlock(&mutex_monitoreo);
		// delegar la conexión al hilo del nivel correspondiente

		//agregar a la lista de listos del nivel
		t_list *p_listos = dictionary_get(listos, mensaje);
		t_pers_por_nivel *item2 = crear_personaje(aux->simbolo, aux->fd);
		pthread_mutex_lock(&mutex_listos);
		list_add(p_listos, item2);
		pthread_mutex_unlock(&mutex_listos);
		//agregar a la lista de listos del nivel

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
		t_monitoreo *item = per_monitor_crear(true, personaje, 0, sockett);
		list_add(personajes_del_sistema, item);
		//estructura: personajes_del_sistema

		enviarMensaje(sockett, ORQ_handshake_PER, "0");

		break;
	}
	case PER_finPlanDeNiveles_ORQ: {

		char personaje = mensaje[0];
		//estructura: personajes en el sistema
		int32_t _esta_personaje(t_pers_koopa *koopa) {
			return koopa->personaje == personaje;
		}
		t_pers_koopa *aux = list_find(personajes_del_sistema,
				(void*) _esta_personaje);
		if (aux != NULL )
			aux->termino_plan = true;
		else
			log_info(logger, "error al buscar el personaje");
		//todo: hacer tratamiento de errores

		//pregunto si _ya_terminaron terminaron todos
		int32_t _ya_terminaron(t_pers_koopa *koopa) {
			return (koopa->termino_plan);
		}
		/*t_list* pendientes = list_filter(personajes_del_sistema,
		 (void*) _esta_pendiente);
		 if (list_is_empty(pendientes))*/
		//bool list_all_satisfy(t_list* self, bool(*condition)(void*));
		if (list_all_satisfy(personajes_para_koopa, (void*) _ya_terminaron))
			log_info(logger, "lanzar_koopa();");
		//list_destroy(pendientes);
		//pregunto si ya terminaron todos
		break;
	}
	default:
		log_info(logger, "mensaje erroneo");
		break;

	}

	free(mensaje);

}
