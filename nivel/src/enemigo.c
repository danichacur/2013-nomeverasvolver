/*
int topeMaximoMapa = 50;
//esta lista de enemigos la agrego para que el nivel los conozca y obtenga sus posiciones para graficarlos
t_list * listaDeEnemigos; //TODO, hacer el create_list
t_list * listaDePersonajes;

void enemigo(){
	t_enemigo * enemigo = crearseASiMismo(); //random, verifica que no se cree en el (0,0)

	while(1){ //cada x tiempo, configurado por archivo de configuracion
		sleep(sleepEnemigos);
		
		if(hayPersonajeAtacable()){
			buscaPersonajeCercano();
			moverseAlternado();
			actualizarUltimoMovimiento();

			if(estoyArribaPersonaje){
				avisarAlNivel();
			}
		}else{
			movermeEnL();
		}
	}
}

t_enemigo * crearseASiMismo(){
	bool igualACeroCero;
	t_enemigo * enemigo;
	while(1){
		enemigo = enemigo_create();
		igualACeroCero = (enemigo->posicion->posX == 0 && enemigo->posicion->posY == 0)
		if(!igualACeroCero)
			break;
	}
	list_add(listaDeEnemigos, enemigo); 
	return enemigo;
}

bool hayPersonajeAtacable(){
	int i;
	int j;
	t_personaje * personaje;
	t_posicion * posicionCaja;
	
	bool hay = false;
	int cantPersonajesEnNivel = list_size(listaDePersonajes);
	int cantCajas = list_size(listaCajas);
		
	if (cantPersonajesEnNivel > 0){
		for(i=0;i<cantPersonajesEnNivel;i++){
			personaje = list_get(listaDePersonajes,i);
			
			j=0;
			while (j<cantCajas && !hay)}{
				posicionCaja = list_get(listaCajas, j);
				
				if (personajeEstaEnCaja(personaje, posicionCaja))
					hay = true;
					
				j++;
			}
		}
	}
	
	return hay;
}

bool personajeEstaEnCaja(t_personaje * personaje, t_posicion * posicionCaja){
	return (personaje->posicion->posX == posicionCaja->posX && personaje->posicion->posY == posicionCaja->posY)
}

////////////////////////////////////////////////////////////////////////////ESTO VA EN OTRO LADO
typedef struct {
        t_posicion * posicion;
} t_enemigo;

typedef struct {
		char * simbolo;
        t_posicion * posicion;
} t_personaje;

t_enemigo * enemigo_create(){
	t_enemigo * enemigo = calloc(sizeof(t_enemigo));
	
	enemigo->posicion = calloc(sizeof(t_posicion));
	enemigo->posicion->posX =  rand() % topeMaximoMapa;
	enemigo->posicion->posX =  rand() % topeMaximoMapa;
	
	return enemigo
}
*/
