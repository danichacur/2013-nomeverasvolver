/*
 * personaje.c
 *
 *  Created on: 05/10/2013
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>


/* Librerias de Commons */
#include <log.h>
#include <config.h>
#include <sockets/sockets.h>
#include <sockets/mensajes.h>
#include <sockets/estructuras.h>

int main (){

	printf("hola mundo!!");

	//ipPuerto_t ipPuerto;
	//ipPuerto.ip = "";
	//ipPuerto.puerto = 0;

	int32_t fd = 1; // = cliente_crearSocketDeConexion(ipPuerto->ip, ipPuerto->puerto);

	char* mensEnvio = "hooola";
	enum tipo_paquete tipoMensEnvio = OK;
	enviarMensaje(fd,tipoMensEnvio,mensEnvio);

	char* mensRta = "";
	enum tipo_paquete tipoMensRta = 0;
	recibirMensaje(fd, tipoMensRta, mensRta);

	printf("%s",nombre_del_enum_paquete(tipoMensRta));
	printf("%s",mensRta);

	return 0;
}

