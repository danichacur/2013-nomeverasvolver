#include "enemigo.h"
#include <commons/log.h>


//esta lista de enemigos la agrego para que el nivel los conozca y obtenga sus posiciones para graficarlos
extern t_list * items;
extern t_list * listaDeEnemigos; //TODO, hacer el create_list
extern int32_t socketDeEscucha;
extern long sleepEnemigos;
extern t_log * logger;
t_list * listaDePersonajes;
char * horizontal;
char * vertical;
char * cadenaVacia;


//#define PRUEBA_CON_CONEXION false
//#define IMPRIMIR_INFO_ENEMIGO true

////////////SEMAFOROS
extern pthread_mutex_t mx_fd;
extern pthread_mutex_t mx_lista_personajes;
extern pthread_mutex_t mx_lista_items;
#include <stdbool.h>

bool IMPRIMIR_INFO_ENEMIGO;

void enemigo(){

	IMPRIMIR_INFO_ENEMIGO = false;
	horizontal = "H";
	vertical = "V";
	cadenaVacia = "";
	//pthread_mutex_init(&mx_fd,NULL);
	//pthread_mutex_init(&mx_lista_personajes,NULL);
	//pthread_mutex_init(&mx_lista_items,NULL);

	t_enemigo * enemigo = crearseASiMismo(); //random, verifica que no se cree en el (0,0)

	if(IMPRIMIR_INFO_ENEMIGO)
		log_info(logger,"Posicion inicial del enemigo. PosX: %d, PosY: %d \n", enemigo->posicion->posX, enemigo->posicion->posY);


	while(1){ //cada x tiempo, configurado por archivo de configuracion
		sleep(sleepEnemigos);

		if(hayPersonajeAtacable()){
			moverseHaciaElPersonajeDeFormaAlternada(enemigo);

			if(estoyArribaDeAlgunPersonaje(enemigo)){
				avisarAlNivel(enemigo);
				//break; //TODO borrar esto, es para la prueba.
			}
		}else{
			movermeEnL(enemigo);
		}
	}

	free(enemigo);
}

t_enemigo * crearseASiMismo(){
	bool igualACeroCero;
	t_enemigo * enemigo;
	while(1){
		enemigo = enemigo_create();
		igualACeroCero = (enemigo->posicion->posX == 0 && enemigo->posicion->posY == 0);
		if(!igualACeroCero)
			break;
	}
	list_add(listaDeEnemigos, enemigo);
	return enemigo;
}

bool hayPersonajeAtacable(){
	bool hay = false;
	t_list * lista = buscarPersonajesAtacables();

	if (list_size(lista)>0)
		hay = true;

	list_clean(lista);
	return hay;
}

t_personaje_niv * moverseHaciaElPersonajeDeFormaAlternada(t_enemigo * enemigo){
	t_personaje_niv * personaje = buscaPersonajeCercano(enemigo);
	if(IMPRIMIR_INFO_ENEMIGO)
		log_info(logger,"Buscando al personaje mas cercano.. Es %s \n", personaje->simbolo);


	char * condicion = estoyEnLineaRectaAlPersonaje(enemigo, personaje); // 'H','V' o ''

	if(condicion == cadenaVacia) //si no estoy en linea recta a la caja,
		if(enemigo->ultimoMovimiento == horizontal)
			moverEnemigoEn(enemigo, personaje, vertical);
		else if(enemigo->ultimoMovimiento == vertical)
			moverEnemigoEn(enemigo, personaje, horizontal);
		else //primer movimiento
			moverEnemigoEn(enemigo, personaje, horizontal);
	else //si estoy en linea recta, condicion me dice la direccion en la que me tengo que mover
		moverEnemigoEn(enemigo, personaje, condicion);

	return personaje;
}

char * estoyEnLineaRectaAlPersonaje(t_enemigo * enemigo, t_personaje_niv * personaje){
	t_posicion * posicionActual = enemigo->posicion;
	t_posicion * posicionBuscada = personaje->posicion;

	char * condicion = cadenaVacia;

	if(posicionActual->posX == posicionBuscada->posX){
		condicion = vertical;

	}else if(posicionActual->posY == posicionBuscada->posY)
		condicion = horizontal;

	return condicion;
}


void moverEnemigoEn(t_enemigo * enemigo, t_personaje_niv * personaje, char * orientacion){
		/* //TODO no estaría esquivando la caja

	if(orientacion == horizontal)
		if(enemigo->posicion->posX > personaje->posicion->posX)
			enemigo->posicion->posX = enemigo->posicion->posX - 1;
		else
			enemigo->posicion->posX = enemigo->posicion->posX + 1;
	else if(orientacion == vertical){
		if(enemigo->posicion->posY > personaje->posicion->posY)
			enemigo->posicion->posY = enemigo->posicion->posY - 1;
		else
			enemigo->posicion->posY = enemigo->posicion->posY + 1;
	}*/


	if(orientacion == horizontal)
		if(enemigo->posicion->posX > personaje->posicion->posX)
			if(hayCaja(enemigo->posicion->posX - 1, enemigo->posicion->posY)){
				orientacion = vertical;
				enemigo->posicion->posY = enemigo->posicion->posY + obtenerDireccionCercaniaEn(orientacion,enemigo,personaje);
			}else
				enemigo->posicion->posX = enemigo->posicion->posX - 1;
		else
			if(hayCaja(enemigo->posicion->posX + 1, enemigo->posicion->posY)){
				orientacion = vertical;
				enemigo->posicion->posY = enemigo->posicion->posY + obtenerDireccionCercaniaEn(orientacion,enemigo,personaje);
			}else
				enemigo->posicion->posX = enemigo->posicion->posX + 1;
	else if(orientacion == vertical){
		if(enemigo->posicion->posY > personaje->posicion->posY)
			if(hayCaja(enemigo->posicion->posX, enemigo->posicion->posY - 1)){
				orientacion = horizontal;
				enemigo->posicion->posX = enemigo->posicion->posX + obtenerDireccionCercaniaEn(orientacion,enemigo,personaje);
			}else
				enemigo->posicion->posY = enemigo->posicion->posY - 1;
		else
			if(hayCaja(enemigo->posicion->posX, enemigo->posicion->posY + 1)){
				orientacion = horizontal;
				enemigo->posicion->posX = enemigo->posicion->posX + obtenerDireccionCercaniaEn(orientacion,enemigo,personaje);
			}else
				enemigo->posicion->posY = enemigo->posicion->posY + 1;
	}


	if(IMPRIMIR_INFO_ENEMIGO)
		log_info(logger,"Posicion del enemigo. PosX: %d, PosY: %d \n", enemigo->posicion->posX, enemigo->posicion->posY);


	//TODO alcanza con esto o tengo q usar una funcion list_replace?
	enemigo->ultimoMovimiento = orientacion;
}

void moverEnemigoEnDireccion(t_enemigo * enemigo, char * orientacion1, int orientacion2){
	/*//TODO no estaría esquivando la caja

	if(orientacion1 == horizontal)
		enemigo->posicion->posX = enemigo->posicion->posX + orientacion2;
	else if(orientacion1 == vertical)
		enemigo->posicion->posY = enemigo->posicion->posY + orientacion2;
	*/

	if(orientacion1 == horizontal)
		if(hayCaja(enemigo->posicion->posX + orientacion2, enemigo->posicion->posY)){
			enemigo->cantTurnosEnL = -1;
			enemigo->posicion->posY = enemigo->posicion->posY + orientacion2;
		}else
			enemigo->posicion->posX = enemigo->posicion->posX + orientacion2;
	else if(orientacion1 == vertical){
		if(hayCaja(enemigo->posicion->posX, enemigo->posicion->posY + orientacion2)){
			enemigo->cantTurnosEnL = -1;
			enemigo->posicion->posX = enemigo->posicion->posX + orientacion2;
		}else
			enemigo->posicion->posY = enemigo->posicion->posY + orientacion2;
	}
	if(IMPRIMIR_INFO_ENEMIGO)
			log_info(logger,"Posicion del enemigo. PosX: %d, PosY: %d \n", enemigo->posicion->posX, enemigo->posicion->posY);

}

bool hayCaja(int x, int y){
	int i = 0;
	bool hay = false;
	tRecursosNivel * recurso;
	t_list * listaRecursosNivel = obtenerListaCajas();

	while(i< list_size(listaRecursosNivel) && !hay){
		recurso = list_get(listaRecursosNivel, i);
		if (recurso->posX == x && recurso->posY == y)
			hay = true;
		i++;
	}

	return hay;
}

int obtenerDireccionCercaniaEn(char * orientacion, t_enemigo * enemigo, t_personaje_niv * personaje){
	int valor;
	if (orientacion == vertical){
		if(enemigo->posicion->posY > personaje->posicion->posY)
			valor = -1;
		else
			valor = 1;
	}else if(orientacion == horizontal){
		if(enemigo->posicion->posX > personaje->posicion->posX)
			valor = -1;
		else
			valor = 1;
	}

	return valor;
}

t_personaje_niv * buscaPersonajeCercano(t_enemigo * enemigo){
	t_list * listaPersonajesAtacables = buscarPersonajesAtacables();
	t_personaje_niv * personajeCercano = list_get(listaPersonajesAtacables,0);
	int distancia1;
	int distancia2;
	int i;

	for(i=0;i<list_size(listaPersonajesAtacables);i++){
		distancia1 = distanciaAPersonaje(enemigo, personajeCercano);
		distancia2 = distanciaAPersonaje(enemigo, list_get(listaPersonajesAtacables,i));

		if(distancia1 > distancia2)
			personajeCercano = list_get(listaPersonajesAtacables,i);
	}

	list_clean(listaPersonajesAtacables);

	return personajeCercano;
}

int distanciaAPersonaje(t_enemigo * enemigo, t_personaje_niv * personaje){
	int distancia;
	distancia = abs(enemigo->posicion->posX - personaje->posicion->posX) +
			abs(enemigo->posicion->posY - personaje->posicion->posY);
	return distancia;
}


t_list * buscarPersonajesAtacables(){
	int i;
	int j;
	t_personaje_niv * personaje;
	tRecursosNivel * caja;
	bool encontrado;
	t_list * listaPersonajesAtacablesUbicaciones;
	t_list * listaRecursosNivel;
	t_list * lista = list_create();

	listaPersonajesAtacablesUbicaciones = obtenerListaPersonajesAtacables();
	listaRecursosNivel = obtenerListaCajas();

	int cantPersonajesEnNivel = list_size(listaPersonajesAtacablesUbicaciones);
	int cantCajas = list_size(listaRecursosNivel);

	if (cantPersonajesEnNivel > 0){
		for(i=0;i<cantPersonajesEnNivel;i++){
			personaje = list_get(listaPersonajesAtacablesUbicaciones,i);

			encontrado = false;
			j=0;
			while (j<cantCajas && !encontrado){
				caja = list_get(listaRecursosNivel, j);

				if (personajeEstaEnCaja(personaje, caja->posX, caja->posY))
					encontrado = true;
				j++;
			}

			if(!encontrado)
				list_add(lista,personaje);

		}
	}

	return lista;
}

t_list * obtenerListaPersonajesAtacables(){
	t_list * personajes = list_create();
	int i;
	for(i=0 ; i < list_size(items) ; i++){
		ITEM_NIVEL * elemento;
		elemento = list_get(items,i);
		if (elemento->item_type == PERSONAJE_ITEM_TYPE){
			t_posicion * pos = posicion_create_pos(elemento->posx, elemento->posy);
			t_personaje_niv * personaje = personaje_create_pos(charToString(elemento->id), pos);
			list_add(personajes, personaje);
		}
	}

	return personajes;
}

t_list * obtenerListaCajas(){
	t_list * cajas = list_create();
	int i;
	for(i=0 ; i < list_size(items) ; i++){
		ITEM_NIVEL * elemento;
		elemento = list_get(items,i);
		if (elemento->item_type == RECURSO_ITEM_TYPE){
			tRecursosNivel * unaCaja = recurso_create("", charToString(elemento->id),elemento->quantity,elemento->posx,elemento->posy);
			list_add(cajas, unaCaja);
		}
	}

	return cajas;
}

bool personajeEstaEnCaja(t_personaje_niv * personaje, int posX, int posY){
	return (personaje->posicion->posX == posX && personaje->posicion->posY == posY);
}

bool estoyArribaDeAlgunPersonaje(t_enemigo * enemigo){
	t_list * lista = obtenerListaDePersonajesAbajoDeEnemigo(enemigo);
	int cantidad = list_size(lista);

	list_clean(lista);
	//free(lista); // TODO se libera t0do????
	return (cantidad > 0);
}

void avisarAlNivel(t_enemigo * enemigo){
	//TODO ver como consigo el fd del Planificador

	int i;
	ITEM_NIVEL * personaje;
	t_list * listaPersonajesAtacados = obtenerListaDePersonajesAbajoDeEnemigo(enemigo);
	char * simbolosPersonajesAtacados = string_new();
	for(i=0 ; i < list_size(listaPersonajesAtacados) ; i++){
		personaje = list_get(listaPersonajesAtacados,i);
		string_append(&simbolosPersonajesAtacados, charToString(personaje->id));
	}

	if(IMPRIMIR_INFO_ENEMIGO)
		log_info(logger,"El enemigo atacó a los personajes: %s \n", simbolosPersonajesAtacados);


	//if (PRUEBA_CON_CONEXION)
	if(true){
		pthread_mutex_lock(&mx_fd);
		enviarMensaje(socketDeEscucha, NIV_enemigosAsesinaron_PLA, simbolosPersonajesAtacados);
		pthread_mutex_unlock(&mx_fd);
	}


	//TODO tengo que sacar los personajes de la lista de personajes?
	while(list_size(listaPersonajesAtacados) > 0){
		ITEM_NIVEL * persAtacado = list_get(listaPersonajesAtacados,0);
		int i = 0;
		bool encontrado = false;
		while(i<list_size(items) && !encontrado){
			ITEM_NIVEL * elem = list_get(items,i);
			if (elem->item_type == PERSONAJE_ITEM_TYPE)
				if (strcmp(charToString(persAtacado->id), charToString(elem->id)) == 0){
					encontrado = true;
					pthread_mutex_lock(&mx_lista_personajes);
					list_remove(items,i);
					//TODO ver si no hay que actulizar el mapa
					pthread_mutex_unlock(&mx_lista_personajes);
				}
			i++;
		}
		list_remove(listaPersonajesAtacados,0);
	}

	free(simbolosPersonajesAtacados);
	list_clean(listaPersonajesAtacados); //TODO, con esto libero todos los elementos de la lista o tengo q recorrerla e ir liberando?

}

t_list * obtenerListaDePersonajesAbajoDeEnemigo(t_enemigo * enemigo){

	int i = 0;
	ITEM_NIVEL * elem;
	t_list * lista = list_create();

	while(i<list_size(items)){
		elem = list_get(items, i);
		if(elem->item_type == PERSONAJE_ITEM_TYPE)
			if (enemigo->posicion->posX == elem->posx && enemigo->posicion->posY == elem->posy)
				list_add(lista, elem);

		i++;
	}

	return lista;
}

void movermeEnL(t_enemigo * enemigo){
	if(IMPRIMIR_INFO_ENEMIGO)
		log_info(logger,"No hay personajes para atacar. Moviendo en L \n");


	char * orientacion;
	int orientacion2;
	if (enemigo->cantTurnosEnL == 0){ //signiifica que es el primer movimiento y tiene que obtener una direccion aleatoria
		int valor = rand() % 2; //TODO, %2 significa 1 y 2  o 0, 1 y 2? Yo necesito solo 2 valores
		if (valor == 1)
			orientacion = horizontal;
		else
			orientacion = vertical;

		int valor2 = rand() % 2; //TODO, %2 significa 1 y 2  o 0, 1 y 2? Yo necesito solo 2 valores
		if (valor2 == 1)
			orientacion2 = 1;
		else
			orientacion2 = -1;

 //void MoverEnemigo(t_list* items, char personaje, int x, int y); para graficar
		moverEnemigoEnDireccion(enemigo, orientacion, orientacion2);
		if(enemigo->cantTurnosEnL > -1){ // si tiene -1 es porque se topó con una caja, y debe arracar la L de vuelta
			enemigo->cantTurnosEnL = 1;
			enemigo->orientacion1 = orientacion;
			enemigo->orientacion2 = orientacion2;
		}else
			enemigo->cantTurnosEnL = 0;
	}else{
		if(enemigo->cantTurnosEnL == 1)
			moverEnemigoEnDireccion(enemigo, enemigo->orientacion1, enemigo->orientacion2);
		else if(enemigo->cantTurnosEnL == 2){
			if(enemigo->orientacion1 == horizontal)
				moverEnemigoEnDireccion(enemigo, vertical, enemigo->orientacion2);
			else
				moverEnemigoEnDireccion(enemigo, horizontal, enemigo->orientacion2);
		}

		if(enemigo->cantTurnosEnL > -1) // si tiene -1 es porque se topó con una caja, y debe arracar la L de vuelta
			enemigo->cantTurnosEnL = enemigo->cantTurnosEnL + 1;

		if(enemigo->cantTurnosEnL == 3 || enemigo->cantTurnosEnL == -1)
			enemigo->cantTurnosEnL = 0;
	}

}



t_enemigo * enemigo_create(){
	t_enemigo * enemigo = malloc(sizeof(t_enemigo));

	//enemigo->posicion = posicion_create_pos_rand(); //TODO le saco que cree random la posicion para realizar pruebas
	enemigo->posicion = posicion_create_pos(1,0);

	enemigo->ultimoMovimiento = "V";
	enemigo->cantTurnosEnL = 0;
	enemigo->orientacion1 = "";
	enemigo->orientacion2 = 0;
	//CrearEnemigo(items,id,posx,posy);

	return enemigo;
}
