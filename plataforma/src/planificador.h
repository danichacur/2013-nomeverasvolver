/*
 * planificador.h
 *
 *  Created on: 08/10/2013
 *      Author: utnso
 */

#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include "orquestador.h"
#include <collections/list.h>

typedef struct{
int32_t nivel;
int32_t fd;
char *algol;
int32_t quantum;
int32_t retardo;
int32_t remain_distance;
} t_niveles_sistema;

void *hilo_planificador(t_niveles_sistema *nivel);
char * transformarListaCadena(t_list *recursosDisponibles);

#endif /* PLANIFICADOR_H_ */
