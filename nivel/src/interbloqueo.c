/*
 * interbloqueo.c
 *
 *  Created on: 03/11/2013
 *      Author: utnso
 */

#include "interbloqueo.h"

int sleepInterbloqueo;
bool recovery;
extern t_list * listaPersonajesRecursos;
int32_t fdPlanificador;
extern pthread_mutex_t mx_fd;

t_personaje_niv1 * personaje_recursos_create(char * simbolo, t_list * recActuales, char * recBloqueante){
	t_personaje_niv1 * new = malloc(sizeof(t_personaje_niv1));
	new->simbolo = simbolo;
	new->recursosActuales = recActuales;
	new->recursoBloqueante = recBloqueante;

	return new;
}

void* rutinaInterbloqueo(){ // funcion rutinaInterbloqueo

	//TODO: HARDCODEADO
	recovery = false;
	sleepInterbloqueo = 15;

	t_list * listaInterbloqueados ;

	while(1){
		sleep(sleepInterbloqueo);
		//log_info(logger, "Checkeo de interbloqueo activado");
		//printf("%s", "Checkeo de interbloqueo activado \n");

		listaInterbloqueados = obtenerPersonajesInterbloqueados();
		if(hayInterbloqueo(listaInterbloqueados)){
			//log_info(logger, "Se detectó un Interbloqueo. Los personajes involucrados son %s", obtenerIdsPersonajes(listaInterbloqueados));
			printf("Se detectó un Interbloqueo. Los personajes involucrados son %s \n", obtenerIdsPersonajes(listaInterbloqueados));

			if(recovery){
				t_personaje_niv1 * personaje = seleccionarVictima(listaInterbloqueados);
				informarVictimaAPlanificador(personaje);
			}

		}//else
			//log_info(logger, "No se detectaron interbloqueos.");
		//	printf("%s", "No se detectaron interbloqueos \n");

	}
	pthread_exit(NULL);
}

t_list * obtenerPersonajesInterbloqueados(){

	t_list * listaBloqueados = obtenerListaDePersonajesBloqueados();

	t_list * listaInterbloqueados = list_create();;
	bool interbloqueo = false;
	int ordenPersActual;

	int j = 0;
	int cantPersonajes = list_size(listaBloqueados);

	while (j<list_size(listaBloqueados)&& !interbloqueo){
	//	listaInterbloqueados = list_create();

		int ordenPrimerPers = j;
		t_personaje_niv1 * primerPers = list_get(listaBloqueados,ordenPrimerPers);
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
			t_personaje_niv1 * pers = list_get(listaBloqueados,i);
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



bool estaEnLista(t_list * lista, t_personaje_niv1 * pers){
	bool esta = false;
	int i;
	for (i=0 ; i<list_size(lista) ; i++){
		t_personaje_niv1 * personaje = list_get(lista,i);
		if(!strcmp(pers->simbolo, personaje->simbolo))
			esta = true;
	}

	return esta;
}

char * obtenerRecursoBloqueante(t_personaje_niv1 * personaje){
	char * recurso = personaje->recursoBloqueante;
	return recurso;
}

bool personajeTieneRecurso(t_personaje_niv1 * personaje, char * recurso){
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

t_personaje_niv1 *seleccionarVictima(t_list *listaInterbloqueados) {

	//encontrar el que primero entro al nivel
	bool _entro_primero(t_personaje_niv1 *primero,
			t_personaje_niv1 *segundo) {
		return primero < segundo;
	}

	list_sort(listaInterbloqueados, (void*) _entro_primero);

	t_personaje_niv1 * pers = list_get(listaInterbloqueados, 0);
	return pers;
}

void informarVictimaAPlanificador(t_personaje_niv1 * personaje){
	pthread_mutex_lock(&mx_fd);
	enviarMensaje(fdPlanificador,NIV_perMuereInterbloqueo_PLA, personaje->simbolo);
	pthread_mutex_unlock(&mx_fd);
}

t_list * obtenerListaDePersonajesBloqueados(){
	t_list * listaPersonajesBloqueados = list_create();

	/*// poner un mutex
	//enviarMensaje(fdPlanificador, NIV_recursosPersonajesBloqueados_PLA, "0");
	char * mensaje;
	//enum tipo_paquete tipoRecibido;

	//: cambiar esto cuando tenga conexion
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

		t_personaje_niv1 * personaje = personaje_recursos_create(recursos[0], listaRecursosActuales, recursos[j]);
		list_add(listaPersonajesBloqueados, personaje);

		i++;
	}*/

	int i;
	for (i=0; i <list_size(listaPersonajesRecursos); i++){
		t_personaje_niv1 * personaje =  list_get(listaPersonajesRecursos,i);
		if (strcmp(personaje->recursoBloqueante, "") != 0)
			list_add(listaPersonajesBloqueados, personaje);
	}

	return listaPersonajesBloqueados;

}

char * obtenerIdsPersonajes(t_list * listaPersonajes){
	char * ids = string_new();
	int i;
	for( i=0 ; i<list_size(listaPersonajes) ; i++){
		t_personaje_niv1 * per = list_get(listaPersonajes,i);
		if(i+1 != list_size(listaPersonajes))
			string_append_with_format(&ids,"%s%s", per->simbolo, ",");
		else
			string_append_with_format(&ids,"%s", per->simbolo);
	}
	return ids;

}
