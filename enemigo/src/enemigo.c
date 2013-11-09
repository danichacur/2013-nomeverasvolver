#include "enemigo.h"


//esta lista de enemigos la agrego para que el nivel los conozca y obtenga sus posiciones para graficarlos
t_list * listaRecursosNivel;
t_list * listaDeEnemigos; //TODO, hacer el create_list
t_list * listaDePersonajes;
char * horizontal;
char * vertical;
char * cadenaVacia;
int sleepEnemigos;

//#define PRUEBA_CON_CONEXION false
//#define IMPRIMIR_INFO_ENEMIGO true

////////////SEMAFOROS
pthread_mutex_t mx_fd;
pthread_mutex_t mx_lista_personajes;
pthread_mutex_t mx_lista_items;

int main(){
	horizontal = "H";
	vertical = "V";
	cadenaVacia = "";
	sleepEnemigos = 1;
	pthread_mutex_init(&mx_fd,NULL);
	pthread_mutex_init(&mx_lista_personajes,NULL);
	pthread_mutex_init(&mx_lista_items,NULL);

	listaRecursosNivel = list_create();
	listaDeEnemigos = list_create();
	listaDePersonajes = list_create();

	/////AGREGO PERSONAJES
	t_personaje * personaje = personaje_create((char *) "@", posicion_create_pos(3,2));
	list_add(listaDePersonajes, personaje);

	t_personaje * personaje2 = personaje_create((char *) "%", posicion_create_pos(6,3));
	list_add(listaDePersonajes, personaje2);

	t_personaje * personaje3 = personaje_create((char *) "#", posicion_create_pos(5,1));
	list_add(listaDePersonajes, personaje3);

	t_personaje * personaje4 = personaje_create((char *) "$", posicion_create_pos(7,4));
	list_add(listaDePersonajes, personaje4);

	/////AGREGO CAJAS
	tRecursosNivel * recurso = recurso_create((char*) "hongos", (char*)"H", 3, 3, 1);
	list_add(listaRecursosNivel,recurso);

	//tRecursosNivel * recurso2 = recurso_create((char*) "monedas", (char*)"M", 3, 4, 2); //este caso con las cajas tan pegadas me rompe
	//list_add(listaRecursosNivel,recurso2);

	tRecursosNivel * recurso3 = recurso_create((char*) "flores", (char*)"F", 3, 6, 3);
	list_add(listaRecursosNivel,recurso3);

	tRecursosNivel * recurso4 = recurso_create((char*) "caracoles", (char*)"C", 3, 5, 5);
	list_add(listaRecursosNivel,recurso4);

	tRecursosNivel * recurso5 = recurso_create((char*) "dolares", (char*)"D", 3, 7, 6);
	list_add(listaRecursosNivel,recurso5);

	enemigo();


	//libero la memoria
	while(!list_is_empty(listaDePersonajes)){
		free(list_remove(listaDePersonajes,0));
	}
	while(!list_is_empty(listaRecursosNivel)){
		free(list_remove(listaRecursosNivel,0));
	}

	return 1; //EXIT_SUCCESS
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////   nivel.c    //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdbool.h>




void enemigo(){
	t_enemigo * enemigo = crearseASiMismo(); //random, verifica que no se cree en el (0,0)

	//if(IMPRIMIR_INFO_ENEMIGO)
		printf("Posicion inicial del enemigo. PosX: %d, PosY: %d \n", enemigo->posicion->posX, enemigo->posicion->posY);


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

t_personaje * moverseHaciaElPersonajeDeFormaAlternada(t_enemigo * enemigo){
	t_personaje * personaje = buscaPersonajeCercano(enemigo);
	//if(IMPRIMIR_INFO_ENEMIGO)
		printf("Buscando al personaje mas cercano.. Es %s \n", personaje->simbolo);


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

char * estoyEnLineaRectaAlPersonaje(t_enemigo * enemigo, t_personaje * personaje){
	t_posicion * posicionActual = enemigo->posicion;
	t_posicion * posicionBuscada = personaje->posicion;

	char * condicion = cadenaVacia;

	if(posicionActual->posX == posicionBuscada->posX){
		condicion = vertical;

	}else if(posicionActual->posY == posicionBuscada->posY)
		condicion = horizontal;

	return condicion;
}


void moverEnemigoEn(t_enemigo * enemigo, t_personaje * personaje, char * orientacion){
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


	//if(IMPRIMIR_INFO_ENEMIGO)
		printf("Posicion del enemigo. PosX: %d, PosY: %d \n", enemigo->posicion->posX, enemigo->posicion->posY);


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
	//if(IMPRIMIR_INFO_ENEMIGO)
			printf("Posicion del enemigo. PosX: %d, PosY: %d \n", enemigo->posicion->posX, enemigo->posicion->posY);

}

bool hayCaja(int x, int y){
	int i = 0;
	bool hay = false;
	tRecursosNivel * recurso;
	while(i< list_size(listaRecursosNivel) && !hay){
		recurso = list_get(listaRecursosNivel, i);
		if (recurso->posX == x && recurso->posY == y)
			hay = true;
		i++;
	}

	return hay;
}

int obtenerDireccionCercaniaEn(char * orientacion, t_enemigo * enemigo, t_personaje * personaje){
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

t_personaje * buscaPersonajeCercano(t_enemigo * enemigo){
	t_list * listaPersonajesAtacables = buscarPersonajesAtacables();
	t_personaje * personajeCercano = list_get(listaPersonajesAtacables,0);
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

int distanciaAPersonaje(t_enemigo * enemigo, t_personaje * personaje){
	int distancia;
	distancia = abs(enemigo->posicion->posX - personaje->posicion->posX) +
			abs(enemigo->posicion->posY - personaje->posicion->posY);
	return distancia;
}


t_list * buscarPersonajesAtacables(){
	int i;
	int j;
	t_personaje * personaje;
	tRecursosNivel * caja;
	bool encontrado;

	t_list * lista = list_create();

	int cantPersonajesEnNivel = list_size(listaDePersonajes);
	int cantCajas = list_size(listaRecursosNivel);

	if (cantPersonajesEnNivel > 0){
		for(i=0;i<cantPersonajesEnNivel;i++){
			personaje = list_get(listaDePersonajes,i);

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

bool personajeEstaEnCaja(t_personaje * personaje, int posX, int posY){
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
	int32_t fdPlanificador = 1;

	int i;
	t_personaje * personaje;
	t_list * listaPersonajesAtacados = obtenerListaDePersonajesAbajoDeEnemigo(enemigo);
	char * simbolosPersonajesAtacados = string_new();
	for(i=0 ; i < list_size(listaPersonajesAtacados) ; i++){
		personaje = list_get(listaPersonajesAtacados,i);
		string_append(&simbolosPersonajesAtacados, personaje->simbolo);
	}

	//if(IMPRIMIR_INFO_ENEMIGO)
		printf("El enemigo atacó a los personajes: %s \n", simbolosPersonajesAtacados);


	//if (PRUEBA_CON_CONEXION)
	if(false){
		pthread_mutex_lock(&mx_fd);
		enviarMensaje(fdPlanificador, NIV_enemigosAsesinaron_PLA, simbolosPersonajesAtacados);
		pthread_mutex_unlock(&mx_fd);
	}


	//TODO tengo que sacar los personajes de la lista de personajes?
	while(list_size(listaPersonajesAtacados) > 0){
		t_personaje * persAtacado = list_get(listaPersonajesAtacados,0);
		int i = 0;
		bool encontrado = false;
		while(i<list_size(listaDePersonajes) && !encontrado){
			t_personaje * pers = list_get(listaDePersonajes,i);
			if (persAtacado->simbolo == pers->simbolo){
				encontrado = true;
				pthread_mutex_lock(&mx_lista_personajes);
				list_remove(listaDePersonajes,i);
				pthread_mutex_unlock(&mx_lista_personajes);

				//TODO Tengo que sacar el elemento de la lista de items. ponerme el semáforo.
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
	t_personaje * personaje;
	t_list * lista = list_create();

	while(i<list_size(listaDePersonajes)){
		personaje = list_get(listaDePersonajes, i);

		if (enemigo->posicion->posX == personaje->posicion->posX && enemigo->posicion->posY == personaje->posicion->posY)
			list_add(lista, personaje);

		i++;
	}

	return lista;
}

void movermeEnL(t_enemigo * enemigo){
	//if(IMPRIMIR_INFO_ENEMIGO)
		printf("No hay personajes para atacar. Moviendo en L \n");


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


