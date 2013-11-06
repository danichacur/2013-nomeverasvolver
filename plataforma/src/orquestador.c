/*
 * orquestador.c
 *
 * Created on: 28/09/2013
 * Author: utnso
 */

#include "orquestador.h"

#define DIRECCION INADDR_ANY //INADDR_ANY representa la direccion de cualquier
//interfaz conectada con la computadora

#define IP "192.168.0.47"
#define PUERTO 4000
#define PATH_CONFIG_ORQ "../orq.conf"

t_dictionary *listos;
t_dictionary *bloqueados;
t_dictionary *anormales;
t_dictionary *monitoreo;

t_config *config;
t_log* logger;
t_list *personajes_para_koopa; // es para lanzar koopa
t_list *personajes_del_sistema; // es para identificar los simbolos de los personajes
t_list *niveles_del_sistema;
int32_t comienzo = 0;

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
	nuevo->ingreso_al_sistema = comienzo;
	comienzo++;
	nuevo->estoy_bloqueado = false;
	nuevo->recursos_obtenidos = list_create();
	return nuevo;
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

void desbloquear_personajes(t_list *recursos_obtenidos, char *str_nivel){

	//t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);

	//todo

}

void supr_pers_de_estructuras(int32_t socket) {
	//si se desconecta un nivel, automaticamente se caen los personajes que estaban conectados
	//si se cae un personaje, el mensaje le llega al planificador directamente

	int32_t _esta_enSistema(t_monitoreo *valor) {
		return valor->fd == socket;
	}
	int32_t _esta_enListas(t_pers_por_nivel *valor) {
		return valor->fd == socket;
	}
	t_monitoreo *aux = list_remove_by_condition(personajes_del_sistema,
			(void*) _esta_enSistema);
	char *str_nivel;

	if (aux == NULL ) {
		//se cayo un nivel
		int32_t _esta_enNiveles(t_niveles_sistema *valor) {
			return valor->fd == socket;
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
	} else {
		//era un personaje lo que se desconecto
		int32_t _esta_enKoopa(t_pers_koopa *valor) {
			return valor->personaje == aux->simbolo;
		}
		t_monitoreo *aux1 = list_remove_by_condition(personajes_para_koopa,
				(void*) _esta_enKoopa);
		free(aux1);
		str_nivel = string_from_format("%d", aux->nivel);
		t_list *p_listos = dictionary_get(listos, str_nivel);
		t_list *p_bloqueados = dictionary_get(bloqueados, str_nivel);
		t_list *p_anormales = dictionary_get(anormales, str_nivel);
		t_list *p_monitoreo = dictionary_get(monitoreo, str_nivel);
		//todo - poner un semaforo aca!
		aux1 = list_remove_by_condition(p_monitoreo, (void*) _esta_enSistema);
		//todo - poner un semaforo aca!
		free(aux1);
		t_pers_por_nivel * aux2 = list_remove_by_condition(p_bloqueados,
				(void*) _esta_enListas);
		desbloquear_personajes(aux2->recursos_obtenidos,str_nivel);
		free(aux2);
		aux2 = list_remove_by_condition(p_anormales, (void*) _esta_enListas);
		desbloquear_personajes(aux2->recursos_obtenidos, str_nivel);
		free(aux2);
		aux2 = list_remove_by_condition(p_listos, (void*) _esta_enListas);
		desbloquear_personajes(aux2->recursos_obtenidos, str_nivel);
		free(aux2);

		//por cada nivel en el que estaba jugando deberia avisarle al nivel!
		//esto lo recibe cada planificador
		//todo enviarMensaje(aux->fd, PLA_personajeDesconectado_NIV, mensaje);
		//por cada nivel en el que estaba jugando deberia avisarle al nivel!
	}
	free(aux);
}

void orquestador_analizar_mensaje(int32_t socket, enum tipo_paquete tipoMensaje,
		char* mensaje);

int main() {

	//inicialización
	//config = config_create(PATH_CONFIG_ORQ);
	//char *PATH_LOG = config_get_string_value(config, "PATH_LOG_ORQ");
	//logger = log_create(PATH_LOG, "ORQUESTADOR", true, LOG_LEVEL_INFO);
	personajes_del_sistema = list_create();
	niveles_del_sistema = list_create();
	personajes_para_koopa = list_create();
	listos = dictionary_create();
	bloqueados = dictionary_create();
	anormales = dictionary_create();
	monitoreo = dictionary_create();
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

	socketEscucha = crearSocketDeConexion(DIRECCION, PUERTO);

	// añadir socketEscucha al conjunto maestro
	FD_SET(socketEscucha, &master);

	// seguir la pista del descriptor de fichero mayor
	fdmax = socketEscucha; // por ahora es éste porque es el primero y unico

	// bucle principal
	printf("entrando al for!! \n");
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
						printf("selectserver: new connection from %s on "
								"socket %d\n", inet_ntoa(remoteaddr.sin_addr),
								newfd);
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
						printf("Llego el tipo de paquete: %s .\n",
								obtenerNombreEnum(tipoMensaje));
						printf("Llego este mensaje: %s .\n", mensaje);

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

void orquestador_analizar_mensaje(int32_t socket, enum tipo_paquete tipoMensaje,
		char* mensaje) {
	switch (tipoMensaje) {

	case NIV_handshake_ORQ: {

		int32_t nivel = atoi(mensaje);

		t_list *p_listos = list_create();
		t_list *p_bloqueados = list_create();
		t_list *p_muertos = list_create();
		t_list *p_monitor = list_create();

		dictionary_put(listos, mensaje, p_listos);
		dictionary_put(bloqueados, mensaje, p_bloqueados);
		dictionary_put(anormales, mensaje, p_muertos);
		dictionary_put(monitoreo, mensaje, p_monitor);

		t_niveles_sistema *nuevo = malloc(sizeof(t_niveles_sistema));
		nuevo->nivel = nivel;
		nuevo->fd = socket;
		list_add(niveles_del_sistema, nuevo);

		printf(
				"Se crea el hilo planificador para el nivel (%d) recien conectado con fd= %d \n",
				nivel, socket);
		pthread_t pla;
		pthread_create(&pla, NULL, (void *) hilo_planificador, nuevo);

		//DELEGAR TMB EL SOCKET AL PLANIFICADOR:
		// delegar la conexión al hilo del nivel correspondiente
		t_list *pe_monitor = dictionary_get(monitoreo, mensaje);
		t_monitoreo *item = per_monitor_crear(false, 'M', atoi(mensaje),
				socket);
		//todo - poner un semaforo aca!
		list_add(pe_monitor, item);
		//todo - poner un semaforo aca!
		// delegar la conexión al hilo del nivel correspondiente

		enviarMensaje(socket, ORQ_handshake_NIV, "0");

		break;
	}
	case PER_conexionNivel_ORQ: {
		int32_t nivel = atoi(mensaje);

		int32_t _esta_personaje(t_monitoreo *nuevo) {
			return nuevo->fd == socket;
		}

		// delegar la conexión al hilo del nivel correspondiente
		printf("nivel solicitado: %d \n", nivel);
		t_monitoreo *aux = list_find(personajes_del_sistema,
				(void*) _esta_personaje);
		aux->nivel = nivel; //actualiza el nivel a donde se conecta el personaje
		t_list *p_monitor = dictionary_get(monitoreo, mensaje);
		t_monitoreo *item = per_monitor_crear(true, 'M', nivel, socket);
		memcpy(item, aux, sizeof(t_monitoreo)); //fixme ver bien esto
		//todo - poner un semaforo aca!
		list_add(p_monitor, item);
		//todo - poner un semaforo aca!
		// delegar la conexión al hilo del nivel correspondiente

		//agregar a la lista de listos del nivel
		t_list *p_listos = dictionary_get(listos, mensaje);
		t_pers_por_nivel *item2 = crear_personaje(aux->simbolo, aux->fd);
		//todo - poner un semaforo aca!
		list_add(p_listos, item2);
		//todo - poner un semaforo aca!
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
			list_add(personajes_para_koopa, item);
		} else {
			if (list_find(personajes_para_koopa,
					(void*) _esta_personaje) == NULL) {
				t_pers_koopa * item = per_koopa_crear(personaje);
				list_add(personajes_para_koopa, item);
			}
		}
		//estructura: personajes_para_koopa

		//estructura: personajes_del_sistema
		t_monitoreo *item = per_monitor_crear(true, personaje, 0, socket);
		//todo - poner un semaforo aca!
		list_add(personajes_del_sistema, item);
		//todo - poner un semaforo aca!
		//estructura: personajes_del_sistema

		enviarMensaje(socket, ORQ_handshake_PER, "0");

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
			printf("error al buscar el personaje");
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
			printf("lanzar_koopa();");
		//list_destroy(pendientes);
		//pregunto si ya terminaron todos
		break;
	}
	default:
		printf("mensaje erroneo");
		break;

	}

	free(mensaje);

}
