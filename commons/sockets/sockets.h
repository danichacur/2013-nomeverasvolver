/*
 * sockets.h
 *
 *  Created on: 06/10/2013
 *      Author: utnso
 */
#ifndef SOCKETS_H_
#define SOCKETS_H_
#include "mensajes.h"

int32_t enviarMensaje (int32_t socket, enum tipo_paquete tipoMensaje, char* mensaje);
int32_t recibirMensaje(int32_t socket, enum tipo_paquete tipoMensaje, char* mensaje);

#endif /* SOCKETS_H_ */
