#include "enemigo.h"
#include <commons/log.h>
#include <stdbool.h>

//esta lista de enemigos la agrego para que el nivel los conozca y obtenga sus posiciones para graficarlos
extern t_list * items;
extern t_list * listaDeEnemigos;
extern int32_t socketDeEscucha;
extern useconds_t sleepEnemigos;
extern t_log * logger;
extern bool graficar;
extern char * nombre;

char * horizontal;
char * vertical;
char * cadenaVacia;


//#define PRUEBA_CON_CONEXION false
//#define IMPRIMIR_INFO_ENEMIGO true

////////////SEMAFOROS
//extern pthread_mutex_t mx_enemigos;
extern pthread_mutex_t mutex_mensajes;
extern pthread_mutex_t mx_lista_personajes;
extern pthread_mutex_t mx_lista_items;
pthread_mutex_t mx_sarasa;
pthread_mutex_t mx_borrar_enemigos;

bool IMPRIMIR_INFO_ENEMIGO;



t_personaje_niv1 * moverseHaciaElPersonajeDeFormaAlternada(t_enemigo * enemigo);
char * estoyEnLineaRectaAlPersonaje(t_enemigo * enemigo, t_personaje_niv1 * personaje);
void moverEnemigoEn(t_enemigo * enemigo, t_personaje_niv1 * personaje, char * orientacion);
int obtenerDireccionCercaniaEn(char * orientacion, t_enemigo * enemigo, t_personaje_niv1 * personaje);
t_personaje_niv1 * buscaPersonajeCercano(t_enemigo * enemigo);
int distanciaAPersonaje(t_enemigo * enemigo, t_personaje_niv1 * personaje);
bool personajeEstaEnCaja(t_personaje_niv1 * personaje, int posX, int posY);
void recurso_destroy(tRecursosNivel * recurso);




void enemigo(int* pIdEnemigo){

	IMPRIMIR_INFO_ENEMIGO = true;
	horizontal = "H";
	vertical = "V";
	cadenaVacia = "";

	int idEnemigo = (int) pIdEnemigo;

	//pthread_mutex_init(&mx_enemigos,NULL);
	//pthread_mutex_init(&mutex_mensajes,NULL);
	//pthread_mutex_init(&mx_lista_personajes,NULL);
	pthread_mutex_init(&mx_borrar_enemigos,NULL);
	pthread_mutex_init(&mx_sarasa,NULL);

	t_enemigo * enemigo = crearseASiMismo(idEnemigo); //random, verifica que no se cree en el (0,0)

	if(IMPRIMIR_INFO_ENEMIGO)
		log_info(logger,"Posicion inicial del enemigo. PosX: %d, PosY: %d ", enemigo->posicion->posX, enemigo->posicion->posY);


	while(1){ //cada x tiempo, configurado por archivo de configuracion
		sleep(sleepEnemigos/1000);

		if(hayPersonajeAtacable()){
			pthread_mutex_lock(&mx_sarasa);
			moverseHaciaElPersonajeDeFormaAlternada(enemigo);
			pthread_mutex_unlock(&mx_sarasa);


			if(estoyArribaDeAlgunPersonaje(enemigo)){
				avisarAlNivel(enemigo);
				//break; //TODO borrar esto, es para la prueba.
			}
		}else{
			/*enemigo->posicion->posX = enemigo->posicion->posX +1;
			pthread_mutex_lock(&mx_lista_items);
			MoverEnemigo(items, enemigo->id, enemigo->posicion->posX,enemigo->posicion->posY);
			if(graficar)			movermeEnL(enemigo);

				nivel_gui_dibujar(items,nombre);
			pthread_mutex_unlock(&mx_lista_items);*/
			pthread_mutex_lock(&mx_sarasa);
			movermeEnL(enemigo);
			pthread_mutex_unlock(&mx_sarasa);

		}

	}

	free(enemigo);
}

t_enemigo * crearseASiMismo(int id){
	bool igualACeroCero;
	t_enemigo * enemigo;
	while(1){
		enemigo = enemigo_create(id);
		igualACeroCero = (enemigo->posicion->posX == 0 && enemigo->posicion->posY == 0);
		if(!igualACeroCero)
			break;
	}

	list_add(listaDeEnemigos, enemigo);
	pthread_mutex_lock(&mx_lista_items);
	CrearEnemigo(items,enemigo->id, enemigo->posicion->posX, enemigo->posicion->posY); //cuidado con esto, el enemigo deberia tener id individuales
	pthread_mutex_unlock(&mx_lista_items);

	if(graficar){
		pthread_mutex_lock(&mx_lista_items);
		nivel_gui_dibujar(items,nombre);
		pthread_mutex_unlock(&mx_lista_items);
	}

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

t_personaje_niv1 * moverseHaciaElPersonajeDeFormaAlternada(t_enemigo * enemigo){
	t_personaje_niv1 * personaje = buscaPersonajeCercano(enemigo);
	if(IMPRIMIR_INFO_ENEMIGO)
		log_info(logger,"Buscando al personaje mas cercano.. Es %s ", personaje->simbolo);


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

char * estoyEnLineaRectaAlPersonaje(t_enemigo * enemigo, t_personaje_niv1 * personaje){
	t_posicion * posicionActual = enemigo->posicion;
	t_posicion * posicionBuscada = personaje->posicion;

	char * condicion = cadenaVacia;

	if(posicionActual->posX == posicionBuscada->posX){
		condicion = vertical;

	}else if(posicionActual->posY == posicionBuscada->posY)
		condicion = horizontal;

	return condicion;
}


void moverEnemigoEn(t_enemigo * enemigo, t_personaje_niv1 * personaje, char * orientacion){
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


	if(orientacion == horizontal){
		if(enemigo->posicion->posX > personaje->posicion->posX){
			if(hayCaja(enemigo->posicion->posX - 1, enemigo->posicion->posY)){
				orientacion = vertical;
				enemigo->posicion->posY = enemigo->posicion->posY + obtenerDireccionCercaniaEn(orientacion,enemigo,personaje);
				MoverEnemigo(items, enemigo->id, enemigo->posicion->posX,enemigo->posicion->posY);
			}else{

				enemigo->posicion->posX = enemigo->posicion->posX - 1;
				MoverEnemigo(items, enemigo->id, enemigo->posicion->posX,enemigo->posicion->posY);

			}
		}else{
			if(hayCaja(enemigo->posicion->posX + 1, enemigo->posicion->posY)){
				orientacion = vertical;
				enemigo->posicion->posY = enemigo->posicion->posY + obtenerDireccionCercaniaEn(orientacion,enemigo,personaje);
				MoverEnemigo(items, enemigo->id, enemigo->posicion->posX,enemigo->posicion->posY);

			}else{
				enemigo->posicion->posX = enemigo->posicion->posX + 1;
				MoverEnemigo(items, enemigo->id, enemigo->posicion->posX,enemigo->posicion->posY);

			}
		}
	}else if(orientacion == vertical){
		if(enemigo->posicion->posY > personaje->posicion->posY)
			if(hayCaja(enemigo->posicion->posX, enemigo->posicion->posY - 1)){
				orientacion = horizontal;
				enemigo->posicion->posX = enemigo->posicion->posX + obtenerDireccionCercaniaEn(orientacion,enemigo,personaje);
				MoverEnemigo(items, enemigo->id, enemigo->posicion->posX,enemigo->posicion->posY);

			}else{
				enemigo->posicion->posY = enemigo->posicion->posY - 1;
				MoverEnemigo(items, enemigo->id, enemigo->posicion->posX,enemigo->posicion->posY);

			}
		else if(hayCaja(enemigo->posicion->posX, enemigo->posicion->posY + 1)){
			orientacion = horizontal;
			enemigo->posicion->posX = enemigo->posicion->posX + obtenerDireccionCercaniaEn(orientacion,enemigo,personaje);
			MoverEnemigo(items, enemigo->id, enemigo->posicion->posX,enemigo->posicion->posY);

		}else
			enemigo->posicion->posY = enemigo->posicion->posY + 1;
			MoverEnemigo(items, enemigo->id, enemigo->posicion->posX,enemigo->posicion->posY);
	}


	if(graficar)
		nivel_gui_dibujar(items,nombre);

	if(IMPRIMIR_INFO_ENEMIGO)
		log_info(logger,"Posicion del enemigo. PosX: %d, PosY: %d ", enemigo->posicion->posX, enemigo->posicion->posY);


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

	if(orientacion1 == horizontal){
		if(hayCajaOExcedeLimite(enemigo->posicion->posX + orientacion2, enemigo->posicion->posY)){
			enemigo->cantTurnosEnL = -1;
			enemigo->posicion->posX = enemigo->posicion->posX + (orientacion2 * -1);
			MoverEnemigo(items, enemigo->id, enemigo->posicion->posX,enemigo->posicion->posY);

		}else{
			enemigo->posicion->posX = enemigo->posicion->posX + orientacion2;
			MoverEnemigo(items, enemigo->id, enemigo->posicion->posX,enemigo->posicion->posY);

		}
	}else if(orientacion1 == vertical){
		if(hayCajaOExcedeLimite(enemigo->posicion->posX, enemigo->posicion->posY + orientacion2)){
			enemigo->cantTurnosEnL = -1;
			enemigo->posicion->posY = enemigo->posicion->posY + (orientacion2 * -1);
			MoverEnemigo(items, enemigo->id, enemigo->posicion->posX,enemigo->posicion->posY);

		}else{
			enemigo->posicion->posY = enemigo->posicion->posY + orientacion2;
			MoverEnemigo(items, enemigo->id, enemigo->posicion->posX,enemigo->posicion->posY);

		}
	}

	if(graficar)
		nivel_gui_dibujar(items,nombre);


	if(IMPRIMIR_INFO_ENEMIGO){
		log_info(logger,"Posicion del enemigo. PosX: %d, PosY: %d ", enemigo->posicion->posX, enemigo->posicion->posY);
	}

}

bool hayCajaOExcedeLimite(int x, int y){
	if (hayCaja(x,y)){
		return true;
	}

	if (excedeLimite(x,y)){
		return true;
	}
	return false;
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

	list_destroy_and_destroy_elements(listaRecursosNivel, (void*)recurso_destroy);

	return hay;
}

bool excedeLimite(int x, int y){
	return (x<0 || y<0);
}

int obtenerDireccionCercaniaEn(char * orientacion, t_enemigo * enemigo, t_personaje_niv1 * personaje){
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

t_personaje_niv1 * buscaPersonajeCercano(t_enemigo * enemigo){
	t_list * listaPersonajesAtacables = buscarPersonajesAtacables();
	t_personaje_niv1 * personajeCercano = list_get(listaPersonajesAtacables,0);
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

int distanciaAPersonaje(t_enemigo * enemigo, t_personaje_niv1 * personaje){
	int distancia;
	distancia = abs(enemigo->posicion->posX - personaje->posicion->posX) +
			abs(enemigo->posicion->posY - personaje->posicion->posY);
	return distancia;
}


t_list * buscarPersonajesAtacables(){
	int i;
	int j;
	t_personaje_niv1 * personaje;
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
			else
				personaje_destroy(personaje);

		}
	}

	list_destroy(listaPersonajesAtacablesUbicaciones);
	list_destroy_and_destroy_elements(listaRecursosNivel, (void*)recurso_destroy);
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

			t_personaje_niv1 * personaje = malloc(sizeof(t_personaje_niv1));
			personaje->posicion = pos;
			personaje->simbolo = charToString(elemento->id);

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

bool personajeEstaEnCaja(t_personaje_niv1 * personaje, int posX, int posY){
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

	pthread_mutex_lock(&mx_borrar_enemigos);
	t_list * listaPersonajesAtacados = obtenerListaDePersonajesAbajoDeEnemigo(enemigo);
	char * simbolosPersonajesAtacados = string_new();
	for(i=0 ; i < list_size(listaPersonajesAtacados) ; i++){
		personaje = list_get(listaPersonajesAtacados,i);
		string_append(&simbolosPersonajesAtacados, charToString(personaje->id));
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
					pthread_mutex_lock(&mx_lista_items);
					list_remove(items,i);
					//TODO ver si no hay que actulizar el mapa
					pthread_mutex_unlock(&mx_lista_items);
				}
			i++;
		}
		list_remove(listaPersonajesAtacados,0);
	}
	pthread_mutex_unlock(&mx_borrar_enemigos);


	if(IMPRIMIR_INFO_ENEMIGO)
		log_info(logger,"El enemigo atacó a los personajes: %s ", simbolosPersonajesAtacados);


	//if (PRUEBA_CON_CONEXION)
	if(true){
		pthread_mutex_lock(&mutex_mensajes);
		enviarMensaje(socketDeEscucha, NIV_enemigosAsesinaron_PLA, simbolosPersonajesAtacados);
		pthread_mutex_unlock(&mutex_mensajes);
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
		log_info(logger,"No hay personajes para atacar. Moviendo en L ");


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



t_enemigo * enemigo_create(int id){

	char * idEnemigo = string_new();
	string_append_with_format(&idEnemigo,"%d",id);

	t_enemigo * enemigo = malloc(sizeof(t_enemigo));

	//enemigo->posicion = posicion_create_pos_rand(); //TODO le saco que cree random la posicion para realizar pruebas
	enemigo->posicion = posicion_create_pos(15,id);

	enemigo->ultimoMovimiento = "V";
	enemigo->cantTurnosEnL = 0;
	enemigo->orientacion1 = "";
	enemigo->orientacion2 = 0;
	enemigo->id=idEnemigo[0];  //agrego el id de enemigo, necesario para las funciones de grafica del nivel

	//pthread_mutex_lock(&mx_enemigos);
	//numeroEnemigo++;
	//pthread_mutex_unlock(&mx_enemigos);


	return enemigo;
}

void recurso_destroy(tRecursosNivel * recurso){
	free(recurso->simbolo);
	free(recurso);
}
