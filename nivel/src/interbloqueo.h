/*
 * interbloqueo.h
 *
 *  Created on: 03/11/2013
 *      Author: utnso
 */

#ifndef INTERBLOQUEO_H_
#define INTERBLOQUEO_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <collections/list.h>
#include <unistd.h>
#include <string.h>
#include <sockets/mensajes.h>
#include <sockets/estructuras.h>
#include <sockets/sockets.h>
#include "nivel.h"




void* rutinaInterbloqueo();
t_list * obtenerPersonajesInterbloqueados();
bool estaEnLista(t_list * lista, t_personaje_niv1 * pers);
char * obtenerRecursoBloqueante(t_personaje_niv1 * personaje);
bool personajeTieneRecurso(t_personaje_niv1 * personaje, char * recurso);
bool hayInterbloqueo(t_list * listaInterbloqueados);
t_personaje_niv1 *seleccionarVictima(t_list *listaInterbloqueados);
void informarVictimaAPlanificador(t_personaje_niv1 * personaje);
t_list * obtenerListaDePersonajesBloqueados();
char * obtenerIdsPersonajes(t_list * listaPersonajes);

#endif /* INTERBLOQUEO_H_ */
