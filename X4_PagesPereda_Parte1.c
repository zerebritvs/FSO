//Juan Antonio Pages Lopez
//Mario Pereda Puyo

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>

//Definicion de tipos de datos
struct dataBuffer
{
	char m_unidades;
	char m_decenas;
	char m_letras;
	bool m_bFin;

};

struct datVector
{
  int m_procesados;
  int m_incorrectos;
  int m_maxIndex;
  
};


//Variables globales
int tamBuffer; 
struct datVector* vector;
struct dataBuffer* buffer1;
char descodificado[100];//Vector que almacena el mensaje descodificado
int indexC; //Indice del Consumidor
int numConsumidores; //Numero de consumidores
int consumidorTerminado;
sem_t hay_Dato, hay_Espacio, mutex_IndexC, hay_DatoC, hay_EspacioC;


// Cuenta el numero de caracteres de cada linea del fichero de entrada
int cuentaStr(char* cadena){
  for(int i=0;i<10;i++){
    if(cadena[i]=='\0'){
        return i;
    }
  
  }
    return -1;
}

//Hilo productor
void *productor (void *arg){

	FILE *fich;
	fich = (FILE *) arg; //fichero de entrada
	char cadena[10];
	struct dataBuffer dato;
  int index = 0;
  
	
	//Lo hace mientras haya caracteres
	while(fscanf(fich, "%s", cadena) != EOF){
        
		//valida si es de 3 caracteres
		if(cuentaStr(cadena) == 3){
 
			dato.m_decenas = cadena[0];//Obtengo la posicion 0 de la cadena
			dato.m_unidades = cadena[1];//Obtengo la posion 1 de la cadena
      dato.m_letras = cadena[2];//Obtengo la posicion 2 de la cadena

			dato.m_bFin = false; //veo si es fin de buffer1
          
			//Escritura en buffer1
			sem_wait(&hay_Espacio);//Espera a que haya espacio en el buffer1
      
			buffer1[index] = dato;//Escribe el dato en la posicion index del buffer
      
			index = (index + 1) % tamBuffer;
			sem_post(&hay_Dato);//Indica que hay dato
          
		}
    }
    
     dato.m_bFin = true;
     sem_wait(&hay_Espacio); // Espera hasta que haya espacio en el buffer1
         
     buffer1[index] = dato;  
     index = (index + 1) % tamBuffer;
     
     sem_post(&hay_Dato);
    
		pthread_exit(NULL);

}

//Hilo consumidor 

void *consumidor(void *arg){	

	struct dataBuffer dato;
  int numTokensValidos = 0;//Tokens validos
  int numTokensDescartados = 0;//Tokens incorrectos
  int maxIndex = 0;//Maximo indice descifrado por cada hilo
	int *id = (int*) arg;
  struct datVector datoCons;

	while(1){
        
		sem_wait(&hay_Dato);// Espera que haya dato
    sem_wait(&mutex_IndexC);
   
		dato = buffer1[indexC];
   
		if((dato.m_bFin) != true){// Si ha llegado al final del buffer1 sale del while
		    indexC = (indexC + 1) % tamBuffer;
		}else{
         sem_post(&mutex_IndexC);//Indica que se puede acceder a IndexC
         sem_post(&hay_Dato);//Indica que hay dato
         break;
    } 
    
		sem_post(&mutex_IndexC);//Indica que se puede acceder a IndexC
    sem_post(&hay_Espacio);//Indica que hay Espacio  

    
		if((dato.m_decenas<='m') && (dato.m_decenas>='d') && (dato.m_unidades<='O') && (dato.m_unidades>='F') && (dato.m_letras<='{') && (dato.m_letras >= '!')){ // Valida token valido
          
      int posicion = ((dato.m_decenas - 100) * 10) + (dato.m_unidades - 70); // Calcula posicion
      descodificado[posicion] = (dato.m_letras - 1); // Guarda en el vector descodificado la letra del mensaje oculto en funcion de la posicion obtenida
      
      if(posicion > maxIndex){ //Calcula el maxIndex
        maxIndex = posicion;
      }
      numTokensValidos++; //Contador de tokens validos 
 
    }else{
      numTokensDescartados++; //Contador de tokens incorrectos
    }
    
   	}	
    
    datoCons.m_procesados = numTokensValidos + numTokensDescartados;//Guarda en struct los tokens procesados del hilo
    datoCons.m_incorrectos = numTokensDescartados;//Guarda en struct los tokens incorrectos del hilo
    datoCons.m_maxIndex = maxIndex;//Guarda en struct el maxIndex del hilo
    
    vector[*id] = datoCons;
    sem_wait(&hay_EspacioC);
    consumidorTerminado = *id;
    sem_post(&hay_DatoC);

	pthread_exit(NULL);
}



//Hilo lector
void *lector(void *arg){

    FILE *fich;
	  fich = (FILE *) arg; //fichero de salida
    int index;
    struct datVector datoCons;
    int totalesProcesados = 0;
    int totalesIncorrectos = 0;
    int maxIndex = 0;
    
    for(int i = 0; i < numConsumidores; i++){//Para cada consumidor
    
      sem_wait(&hay_DatoC);//Espera hasta que haya dato para el consumidor
      
      index = consumidorTerminado;
      datoCons = vector[index];
      
      sem_post(&hay_EspacioC);//Indica que hay espacio para el consumidor
      
      totalesProcesados = totalesProcesados + datoCons.m_procesados;//Obtiene los tokens procesados totales
      totalesIncorrectos = totalesIncorrectos + datoCons.m_incorrectos;//Obtiene los tokens incorrectos totales
      
      fprintf(fich, "Hilo %d:\n\tTokens procesados: %d\n\tTokens correctos: %d\n\tTokens incorrectos: %d\n\tMaxIndex: %d\n", index, datoCons.m_procesados, (datoCons.m_procesados-datoCons.m_incorrectos), datoCons.m_incorrectos, datoCons.m_maxIndex);
      
      if(datoCons.m_maxIndex > maxIndex){//Obtiene el maxIndex global
          maxIndex = datoCons.m_maxIndex;      
      }
    
    }
    
    fprintf(fich, "\nResultado final (los que procesa el consumidor final).\n\tTokens procesados: %d\n\tTokens correctos: %d\n\tTokens incorrectos: %d\n\tMaxIndex: %d\n\t", totalesProcesados, (totalesProcesados-totalesIncorrectos), totalesIncorrectos, maxIndex);
    
    if(maxIndex == ((totalesProcesados-totalesIncorrectos)-1)){//Comprueba si el mensaje es correcto
        fprintf(fich, "Mensaje: Correcto\n\n");
    }else{
        fprintf(fich, "Mensaje: Incorrecto\n\n");
    }
    
    fprintf(fich, "Mensaje traducido: " );
    for(int i = 0; i<100; i++){
    fprintf(fich, "%c", descodificado[i]);
  }
  fprintf(fich, "\n");
  
  pthread_exit(NULL);
}



//main

int main(int argc, char *argv[])
{	

	FILE *fichInput;
  FILE *fichOutput;
	
	//valida si el numero de argumentos pasados al programa es 5
	if(argc != 5){
		printf("Numero de argumentos no valido");	
		return -1;
	}
	
	fichInput = fopen(argv[1], "r");
  
  //valida el fichero de entrada
	if(fichInput == NULL){
    printf("El archivo de entrada esta vacio");
    return -1;
  }
  fichOutput = fopen(argv[2], "w");
  
  //valida el fichero de salida
  if(fichOutput == NULL){
    printf("El archivo de salida esta vacio");
    return -1;
  
	}
	//validacion para el tercer argumento (tamBuffer)
	if((sscanf(argv[3], "%d",&tamBuffer) == 0)){
    printf("Dimension de buffer no valido");
		return -1;

	}else{
	  
	  buffer1 = (struct dataBuffer*) malloc(tamBuffer * sizeof(struct dataBuffer));//Reserva memoria al buffer1
   }
  //valida el argumento 4 del programa (nuemro de consumidores)
  if(sscanf(argv[4], "%d", &numConsumidores) < 1){
    printf("Numero de Consumidores no valido");
    return -1;
  }
  
  //valida la reserva de memoria para el buffer
	if(buffer1 == NULL){
		printf("Error al asignar memoria para el buffer circular");
		return -1;
	}
 
  vector = (struct datVector*)malloc(numConsumidores*sizeof(struct datVector));   
  
  //valida la reserva de memoria para el vector
  if(vector == NULL){
		printf("Error al asignar memoria para el vector");
		return -1;
	}
	//Inicializacion de los semaforos
	sem_init(&hay_Dato, 0, 0);
	sem_init(&hay_Espacio, 0, tamBuffer);
  sem_init(&mutex_IndexC, 0, 1);
  sem_init(&hay_DatoC, 0, 0);
  sem_init(&hay_EspacioC, 0, 1);
  
	//Creacion de los hilos
	pthread_t tProductor, tConsumidor[numConsumidores], tLector;
 
	pthread_create(&tProductor, NULL, productor, (void*)fichInput);
 
	int I[numConsumidores];
 
  for(int i = 0; i < numConsumidores; i++){
    I[i] = i;
    pthread_create(&tConsumidor[i], NULL, consumidor, (void*) &I[i]);
  
  }
  pthread_create(&tLector, NULL, lector, (void*)fichOutput);

	//Espera a que acaben ambos hilos
	pthread_join(tProductor,NULL);
 
  for(int i = 0; i < numConsumidores; i++){
    pthread_join(tConsumidor[i], NULL);
  
  }
  pthread_join(tLector, NULL);

	//Destruye semaforos 
  sem_destroy(&hay_Dato);
  sem_destroy(&hay_Espacio);
  sem_destroy(&mutex_IndexC);
  sem_destroy(&hay_EspacioC);
  sem_destroy(&hay_DatoC);
  
  //Cierra ficheros
  fclose(fichInput);
  fclose(fichOutput);
  
  //Libera memoria
  free(vector);
	free(buffer1);


	return 1;// si el programa funciona correctamente devuelve 1

}	
