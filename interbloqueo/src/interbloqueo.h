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


typedef struct {
    char * simbolo;
    t_posicion * posicion;

    t_list * recursosActuales;
	char * recursoBloqueante;
} t_personaje;


typedef struct {
        char* nombre;
        char* simbolo;
        int instancias;
        int posX;
        int posY;
}tRecursosNivel;

t_personaje * personaje_rec_create(char * simbolo, t_list * recActuales, char * recBloqueante){
	t_personaje * new = malloc(sizeof(t_personaje));
	new->simbolo = simbolo;
	new->recursosActuales = recActuales;
	new->recursoBloqueante = recBloqueante;

	return new;
}


t_list * obtenerPersonajesInterbloqueados();
bool estaEnLista(t_list * lista, t_personaje * pers);
char * obtenerRecursoBloqueante(t_personaje * personaje);
bool personajeTieneRecurso(t_personaje * personaje, char * recurso);
bool hayInterbloqueo(t_list * listaInterbloqueados);
t_personaje * seleccionarVictima(t_list * listaInterbloqueados);
void informarVictimaAPlanificador(t_personaje * personaje);
t_list * obtenerListaDePersonajesDePlanificador();
char * obtenerIdsPersonajes(t_list * listaPersonajes);

#endif /* INTERBLOQUEO_H_ */
