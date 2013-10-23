/*
 * planificador.h
 *
 *  Created on: 08/10/2013
 *      Author: utnso
 */

#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include "orquestador.h"

typedef struct{
int32_t nivel;
int32_t fd;
} t_niveles_sistema;

void *hilo_planificador(t_niveles_sistema *nivel);

#endif /* PLANIFICADOR_H_ */
