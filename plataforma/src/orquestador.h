/*
 * orquestador.h
 *
 *  Created on: 08/10/2013
 *      Author: utnso
 */

#ifndef ORQUESTADOR_H_
#define ORQUESTADOR_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h> // aca esta memset
#include <pthread.h>
#include <stdbool.h>
#include "planificador.h"

/* Librerias de Commons */
#include <log.h>
#include <config.h>
#include <string.h>
#include <collections/list.h>

#include <sockets/sockets.h>
#include <sockets/mensajes.h>

/* Esto es para el select() */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/* Esto es para las conexiones */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


typedef struct{
	char personaje;
	int32_t fd;
	bool termino_plan;
} t_pers_sistema;

typedef struct{
	char personaje;
	int32_t fd;
	int32_t ingreso_al_sistema;
	char recurso_bloqueo;
	bool estoy_bloqueado;
} t_pers_por_nivel;

typedef struct{
	int32_t nivel;
	int32_t fd;
} t_niveles_sistema;

#endif /* ORQUESTADOR_H_ */
