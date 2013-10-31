/*
* orquestador.h
*
* Created on: 08/10/2013
* Author: utnso
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
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
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
bool termino_plan;
} t_pers_koopa;

typedef struct{
char personaje;
int32_t fd;
int32_t ingreso_al_sistema;
char recurso_bloqueo;
bool estoy_bloqueado;
t_list *recursos_obtenidos;
} t_pers_por_nivel;

typedef struct{
	char recurso;
	int32_t cantidad;
}t_recursos_obtenidos;

typedef struct{
bool es_personaje;
char simbolo;
int32_t nivel;
int32_t fd;
} t_monitoreo;

void supr_pers_de_estructuras (int32_t socket);
void desbloquear_personajes(t_list *recursos_obtenidos);

#endif /* ORQUESTADOR_H_ */
