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
int32_t recibirMensaje(int32_t socket, enum tipo_paquete *tipoMensaje, char **mensaje);
int32_t crearSocketDeConexion(int32_t DIRECCION, int32_t PUERTO);
int32_t cliente_crearSocketDeConexion(char *DIRECCION, int32_t PUERTO);


#endif /* SOCKETS_H_ */
