/*
 * planificador.h
 *
 *  Created on: 08/10/2013
 *      Author: utnso
 */

#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include "orquestador.h"
#include <sockets/mensajes.h>
#include <collections/list.h>

typedef struct{
int32_t nivel;
int32_t fd;
char *algol;
pthread_t pla;
int32_t quantum;
int32_t retardo;
int32_t remain_distance;
t_list *pers_conectados;
t_list *pers_desconectados;
int cant_pers;
pthread_mutex_t mutex_inicial;
pthread_mutex_t mutex_lista;
char* str_nivel;
} t_niveles_sistema;

void *hilo_planificador(t_niveles_sistema *nivel);
char * transformarListaCadena(t_list *recursosDisponibles);
bool plan_enviarMensaje(t_niveles_sistema* nivel, int32_t fd, enum tipo_paquete paquete, char* mensaje);
void agregar_anormales(char* str_nivel, int32_t fd);

#endif /* PLANIFICADOR_H_ */
