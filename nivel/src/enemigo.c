/*

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////   nivel.h    //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct {
        t_posicion * posicion;
        char * ultimoMovimiento;
        int cantTurnosEnL;
        char * orientacion1;
        int orientacion2;
} t_enemigo;

typedef struct {
		char * simbolo;
        t_posicion * posicion;
} t_personaje;


t_enemigo * enemigo_create(){
	t_enemigo * enemigo = malloc(sizeof(t_enemigo));

	enemigo->posicion = malloc(sizeof(t_posicion));
	enemigo->posicion->posX =  rand() % topeMaximoMapa;
	enemigo->posicion->posX =  rand() % topeMaximoMapa;

	enemigo->ultimoMovimiento = "V";
	enemigo->cantTurnosEnL = 0;
	enemigo->orientacion1 = "";
	enemigo->orientacion2 = 0;

	return enemigo;
}


t_enemigo * crearseASiMismo();
bool hayPersonajeAtacable();
t_personaje * moverseHaciaElPersonajeDeFormaAlternada(t_enemigo * enemigo);
char * estoyEnLineaRectaAlPersonaje(t_enemigo * enemigo, t_personaje * personaje);
void moverEnemigoEn(t_enemigo * enemigo, t_personaje * personaje, char * orientacion);
void moverEnemigoEnDireccion(t_enemigo * enemigo, char * orientacion1, int orientacion2);
t_personaje * buscaPersonajeCercano(t_enemigo * enemigo);
int distanciaAPersonaje(t_enemigo * enemigo, t_personaje * personaje);
t_list * buscarPersonajesAtacables();
bool personajeEstaEnCaja(t_personaje * personaje, int posX, int posY);
bool estoyArribaPersonaje(t_enemigo * enemigo, t_personaje * personaje);
void avisarAlNivel(t_personaje * personajeAtacado);
void movermeEnL(t_enemigo * enemigo);


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////   nivel.c    //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
int topeMaximoMapa = 50;
#include <stdbool.h>


//esta lista de enemigos la agrego para que el nivel los conozca y obtenga sus posiciones para graficarlos
t_list * listaDeEnemigos; //TODO, hacer el create_list
t_list * listaDePersonajes;
char * horizontal = "H";
char * vertical = "V";


void enemigo(){
	t_enemigo * enemigo = crearseASiMismo(); //random, verifica que no se cree en el (0,0)
	t_personaje * personajeCercano;
	while(1){ //cada x tiempo, configurado por archivo de configuracion
		//sleep(sleepEnemigos);

		if(hayPersonajeAtacable()){
			personajeCercano = moverseHaciaElPersonajeDeFormaAlternada(enemigo);

			if(estoyArribaPersonaje(enemigo,personajeCercano)){
				avisarAlNivel(personajeCercano);
			}
		}else{
			movermeEnL(enemigo);
		}
	}
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

	if (list_size(buscarPersonajesAtacables())>0)
		hay = true;

	return hay;
}

t_personaje * moverseHaciaElPersonajeDeFormaAlternada(t_enemigo * enemigo){
	t_personaje * personaje = buscaPersonajeCercano(enemigo);

	char * condicion = estoyEnLineaRectaAlPersonaje(enemigo, personaje); // 'H','V' o ''

	if(condicion == string_new()) //si no estoy en linea recta a la caja,
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

	char * condicion = string_new();

	if(posicionActual->posX == posicionBuscada->posX){
		condicion = "H";

	}else if(posicionActual->posY == posicionBuscada->posY)
		condicion = "V";

	return condicion;
}


void moverEnemigoEn(t_enemigo * enemigo, t_personaje * personaje, char * orientacion){
		//TODO no estaría esquivando la caja

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
	}
	//TODO alcanza con esto o tengo q usar una funcion list_replace?
	enemigo->ultimoMovimiento = orientacion;
}

void moverEnemigoEnDireccion(t_enemigo * enemigo, char * orientacion1, int orientacion2){
	//TODO no estaría esquivando la caja

	if(orientacion1 == horizontal)
		enemigo->posicion->posX = enemigo->posicion->posX + orientacion2;
	else if(orientacion1 == vertical)
		enemigo->posicion->posY = enemigo->posicion->posY + orientacion2;

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

	return personajeCercano;
}

int distanciaAPersonaje(t_enemigo * enemigo, t_personaje * personaje){
	int distancia;
	//TODO buscar fórmula de valorAbsoluto
	distancia = 2;
	//distancia = valorAbsoluto(enemigo->posicion->posX - personaje->posicion->posX) +
	//		valorAbsoluto(enemigo->posicion->posY - personaje->posicion->posY);
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

				if (!personajeEstaEnCaja(personaje, atoi(caja->posX), atoi(caja->posY))){
					list_add(lista,personaje);
					encontrado = true;
				}
				j++;
			}
		}
	}

	return lista;
}

bool personajeEstaEnCaja(t_personaje * personaje, int posX, int posY){
	return (personaje->posicion->posX == posX && personaje->posicion->posY == posY);
}

bool estoyArribaPersonaje(t_enemigo * enemigo, t_personaje * personaje){
	bool estoy = false;
	if (enemigo->posicion->posX == personaje->posicion->posX &&
			enemigo->posicion->posY == personaje->posicion->posY)
		estoy = true;
	return estoy;
}

void avisarAlNivel(t_personaje * personajeAtacado){
	//TODO ver como consigo el fd del Planificador
	int32_t fdPlanificador = 1;

	char * mensaje = string_new();
	string_append(&mensaje, personajeAtacado->simbolo);
	enviarMensaje(fdPlanificador, NIV_enemigosAsesinaron_PLA, mensaje);

}

void movermeEnL(t_enemigo * enemigo){
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
		enemigo->cantTurnosEnL = 1;
		enemigo->orientacion1 = orientacion;
		enemigo->orientacion2 = orientacion2;
	}else{
		if(enemigo->cantTurnosEnL == 1)
			moverEnemigoEnDireccion(enemigo, enemigo->orientacion1, enemigo->orientacion2);
		else if(enemigo->cantTurnosEnL == 2){
			if(enemigo->orientacion1 == horizontal)
				moverEnemigoEnDireccion(enemigo, vertical, enemigo->orientacion2);
			else
				moverEnemigoEnDireccion(enemigo, horizontal, enemigo->orientacion2);
		}

		enemigo->cantTurnosEnL = enemigo->cantTurnosEnL + 1;

		if(enemigo->cantTurnosEnL == 4)
			enemigo->cantTurnosEnL = 0;
	}

}


*/
