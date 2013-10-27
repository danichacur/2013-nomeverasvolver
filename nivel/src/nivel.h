/*
 * nivel.h
 *
 *  Created on: 23/10/2013
 *      Author: utnso
 */

#ifndef NIVEL_H_
#define NIVEL_H_

////////////////////////////////////////////////////BIBLIOTECAS ESTANDAR////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <unistd.h>
#include <string.h>

////////////////////////////////////////////////////BIBLIOTECAS COMMONS////////////////////////////////////////////////////
#include <sockets/sockets.h>
#include <sockets/mensajes.h>
#include <sockets/estructuras.h>
#include <commons/temporal.h>
#include <collections/list.h>
#include <commons/config.h>
#include <pthread.h>
#include "nivel.h"
//#include <tad_items.h>

////////////////////////////////////////////////////ESPACIO DE DEFINICIONES////////////////////////////////////////////////////


typedef struct {
	char** nombre;
	char** simbolo;
	char** instancias;
	char** posX;
	char** posY;
}tListaItems;


void buscaPersonajeCercano();
void moverseAlternado();
void actualizarUltimoMovimiento();
void crearseASiMismo();
void movermeEnL();
int main();
void crearCaja(t_list *listaCajas ,char ** elemento1,char ** elemento2,char ** elemento3,char ** elemento4,char ** elemento5);
int leerArchivoConfiguracion();
void conectarmeConPlataforma(); // handshake inicial
void enviarDatosInicioNivel(); //envia algoritmo,quantum y retardo
void crearHilosEnemigos();
void crearHiloInterbloqueo();
void crearHiloInotify();
void procesarSolicitudesPlanificador(int32_t socket, enum tipo_paquete tipoMensaje,char* mensaje);


#endif /* NIVEL_H_ */
