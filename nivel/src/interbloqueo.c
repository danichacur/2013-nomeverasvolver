/*
 * interbloqueo.c
 *
 *  Created on: 03/11/2013
 *      Author: utnso
 */

#include "interbloqueo.h"

int sleepInterbloqueo;
bool recovery;
extern t_list * listaDePersonajes;
int32_t fdPlanificador;

t_personaje * personaje_recursos_create(char * simbolo, t_list * recActuales, char * recBloqueante){
	t_personaje * new = malloc(sizeof(t_personaje));
	new->simbolo = simbolo;
	new->recursosActuales = recActuales;
	new->recursoBloqueante = recBloqueante;

	return new;
}

void* rutinaInterbloqueo(){ // funcion rutinaInterbloqueo

	//TODO: HARDCODEADO
	recovery = true;
	sleepInterbloqueo = 3;

	t_list * listaInterbloqueados ;

	while(1){
		sleep(sleepInterbloqueo);
		//log_info(logger, "Checkeo de interbloqueo activado");
		printf("%s", "Checkeo de interbloqueo activado \n");

		listaInterbloqueados = obtenerPersonajesInterbloqueados();
		if(hayInterbloqueo(listaInterbloqueados)){
			//log_info(logger, "Se detectó un Interbloqueo. Los personajes involucrados son %s", obtenerIdsPersonajes(listaInterbloqueados));
			printf("Se detectó un Interbloqueo. Los personajes involucrados son %s \n", obtenerIdsPersonajes(listaInterbloqueados));

			if(recovery){
				t_personaje * personaje = seleccionarVictima(listaInterbloqueados);
				informarVictimaAPlanificador(personaje);
			}

		}else
			//log_info(logger, "No se detectaros interbloqueos.");
			printf("%s", "No se detectaros interbloqueos \n");

	}
	pthread_exit(NULL);
}

t_list * obtenerPersonajesInterbloqueados(){

	t_list * listaBloqueados = obtenerListaDePersonajesBloqueados();

	t_list * listaInterbloqueados;
	bool interbloqueo = false;
	int ordenPersActual;

	int j = 0;
	int cantPersonajes = list_size(listaBloqueados);

	while (j<list_size(listaBloqueados)&& !interbloqueo){
		listaInterbloqueados = list_create();

		int ordenPrimerPers = j;
		t_personaje * primerPers = list_get(listaBloqueados,ordenPrimerPers);
		char * recursoBloqueantePrimerPers = obtenerRecursoBloqueante(primerPers);

		int i;
		if(j + 1 == list_size(listaBloqueados))
			i = 0;
		else
			i = j + 1;

		ordenPersActual = j;
		char * recursoBloqueanteActual = string_new();
		strcpy(recursoBloqueanteActual,recursoBloqueantePrimerPers);
		list_add(listaInterbloqueados, list_get(listaBloqueados,j));
		while (i != ordenPersActual && !interbloqueo){
			t_personaje * pers = list_get(listaBloqueados,i);
			if(personajeTieneRecurso(pers, recursoBloqueanteActual) && !estaEnLista(listaInterbloqueados,pers)){
				list_add(listaInterbloqueados, pers);
				recursoBloqueanteActual = obtenerRecursoBloqueante(pers);
				ordenPersActual = i;
			}else if(personajeTieneRecurso(pers, recursoBloqueanteActual) && estaEnLista(listaInterbloqueados,pers))
				if(!strcmp(recursoBloqueantePrimerPers,obtenerRecursoBloqueante(pers)))
					interbloqueo = true;

			if(i+1==cantPersonajes)
				i = 0;
			else
				i++;
		}

		if(!interbloqueo)
			list_clean(listaInterbloqueados);

		j++;
	}
	list_clean(listaBloqueados);
	return listaInterbloqueados;
}



bool estaEnLista(t_list * lista, t_personaje * pers){
	bool esta = false;
	int i;
	for (i=0 ; i<list_size(lista) ; i++){
		t_personaje * personaje = list_get(lista,i);
		if(!strcmp(pers->simbolo, personaje->simbolo))
			esta = true;
	}

	return esta;
}

char * obtenerRecursoBloqueante(t_personaje * personaje){
	char * recurso = personaje->recursoBloqueante;
	return recurso;
}

bool personajeTieneRecurso(t_personaje * personaje, char * recurso){
	int i = 0;
	bool encontrado = false;
	while(i < list_size(personaje->recursosActuales) && !encontrado){
		if (!strcmp(recurso,list_get(personaje->recursosActuales,i)))
			encontrado = true;
		i++;
	}
	return encontrado;
}


bool hayInterbloqueo(t_list * listaInterbloqueados){
	return (list_size(listaInterbloqueados) > 0); // toma el primero que se conectó porque está en orden.
}

t_personaje * seleccionarVictima(t_list * listaInterbloqueados){
	t_personaje * pers = list_get(listaInterbloqueados,0);
	return pers;
}

void informarVictimaAPlanificador(t_personaje * personaje){
	//enviarMensaje(fdPlanificador,NIV_perMuereInterbloqueo_PLA, personaje->simbolo);
}

t_list * obtenerListaDePersonajesBloqueados(){
	t_list * listaPersonajesBloqueados = list_create();

	/*//TODO poner un mutex
	//enviarMensaje(fdPlanificador, NIV_recursosPersonajesBloqueados_PLA, "0");
	char * mensaje;
	//enum tipo_paquete tipoRecibido;

	//TODO: cambiar esto cuando tenga conexion
	//recibirMensaje(fdPlanificador, &tipoRecibido, &mensaje);
	//tipoRecibido = PLA_recursosPersonajesBloqueados_NIV;
	mensaje = "$,T,J;#,H,F,M;@,J,H,F;%,F,M,H"; //$,T,J;


	char ** personajes = string_split(mensaje,";");
	int i=0;
	while(personajes[i] != NULL){
		char ** recursos = string_split(personajes[i], ",");
		t_list * listaRecursosActuales = list_create();
		int j = 1;
		while(recursos[j + 1] != NULL){
			list_add(listaRecursosActuales, recursos[j]);
			j++;
		}

		t_personaje * personaje = personaje_recursos_create(recursos[0], listaRecursosActuales, recursos[j]);
		list_add(listaPersonajesBloqueados, personaje);

		i++;
	}*/

	int i;
	for (i=0; i <list_size(listaDePersonajes); i++){
		t_personaje * personaje =  list_get(listaDePersonajes,i);
		if (strcmp(personaje->recursoBloqueante, "") != 0)
			list_add(listaPersonajesBloqueados, personaje);
	}

	return listaPersonajesBloqueados;

}

char * obtenerIdsPersonajes(t_list * listaPersonajes){
	char * ids = string_new();
	int i;
	for( i=0 ; i<list_size(listaPersonajes) ; i++){
		t_personaje * per = list_get(listaPersonajes,i);
		if(i+1 != list_size(listaPersonajes))
			string_append_with_format(&ids,"%s%s", per->simbolo, ",");
		else
			string_append_with_format(&ids,"%s", per->simbolo);
	}
	return ids;

}