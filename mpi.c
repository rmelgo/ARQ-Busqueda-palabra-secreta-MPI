#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "mpi.h"
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#define CHAR_NF 32
#define CHAR_MAX 127
#define CHAR_MIN 33
#define PESO_COMPROBAR 5000000
#define PESO_GENERAR 10000000

typedef struct {
	int num_iteraciones;
	double tiempo_total;
	double tiempo_generar;
	double tiempo_espera_comprobar;
} estadisticas_generador;

typedef struct {
	int num_iteraciones;
	double tiempo_total;
	double tiempo_comprobar;
} estadisticas_comprobador;

char * genera_palabra();
char * genera_palabra_con_pista(int tamPalabra, char *vectorPista);
void fuerza_espera(unsigned long peso);
void construir_tipo_derivado_comprobar(estadisticas_comprobador * pdatos, MPI_Datatype * pMPI_Tipo_Datos);
void construir_tipo_derivado_generar(estadisticas_generador * pdatos, MPI_Datatype * pMPI_Tipo_Datos);

int main(int argc, char** argv)
{
	int myrank,nprocs,i,j;
	int etiqueta_notificacion_tipo = 50;
	int etiqueta_palabra = 55;
	int etiqueta_pista = 60;
	int etiqueta_estadisticas_comprobador = 65;
	int etiqueta_estadisticas_generador = 70;
	int recibido1,recibido2;
	int numcomprobadores;
	int tipoProceso;
	char * key;
	int tamPalabra = 50; //definicion del tamaño de la palabra
	double tinicial, tfinal;
	
	MPI_Status statusmensaje;
	MPI_Request peticion1;
	MPI_Request peticion2;

	MPI_Init(&argc, &argv); //inicializamos el entorno de MPI

	MPI_Comm_size(MPI_COMM_WORLD,&nprocs); //obtenemos el numero de procesos

	MPI_Comm_rank(MPI_COMM_WORLD,&myrank); //obtenemos el identificador del proceso


	//COMPROBACIONES INICIALES

	if (nprocs < 3) {
		if (myrank == 0) {
    		printf("Se requieren al menos 3 procesos para ejecutar el programa (un proceso de E/S, un proceso Generador y un proceso Comprobador)\n");
    	}
    	MPI_Finalize();
    	return 0;
	}
	if (argc != 3) 
	{
		if (myrank == 0) {
			printf("Numero de argumentos incorrecto (mpirun -np <nombre_ejecutable> <numero_comprobadores> <0 o 1 (0 -> no pista y 1 -> pista)>\n");
		}
		MPI_Finalize();
		return 0;
	}
	else
	{
		numcomprobadores = atoi(argv[1]);
		if ((nprocs - numcomprobadores) < 2) {
			if (myrank == 0) {
				printf("Numero de procesos generadores insuficiente\n");
			}
			MPI_Finalize();
			return 0;
		}
	}
	
	//El proceso de E/S asigna los distintos roles a los procesos y a los comprobadores les envia la palabra

	if (myrank == 0)  
	{	
		printf("=============================================================================\n");
		printf("BUSQUEDA CADENA ALEATORIA SECRETA\n");
		printf("=============================================================================\n\n");
		//printf("\n");
		if (atoi(argv[2]) == 0) {
			printf("----------MODO NO PISTA----------\n");
		} else {
			printf("----------MODO PISTA----------\n");
		}

    	printf("\nNUMERO DE PROCESOS: Total %d: E/S: 1, Comprobadores: %d, Generadores: %d\n", nprocs, numcomprobadores, nprocs - numcomprobadores - 1);
    	printf("\nNOTIFICACION TIPO");
    	
    	//Envio del rol a los comprobadores
    	
    	for (i = 1; i <= numcomprobadores; i++)
    	{
        	tipoProceso = 0;
        	MPI_Send(&tipoProceso, 1, MPI_INT, i, etiqueta_notificacion_tipo, MPI_COMM_WORLD);  
        	printf("\n0%d) %d", i, tipoProceso);
    	}
    	
    	//Envio del rol a los generadores (indicando su respectivo comprobador)

    	while (i < nprocs) {
        	for (j = 1; j <= numcomprobadores; j++)
        	{
        		if (i >= nprocs) {
         			break;
         		}
            	tipoProceso = j;
            	MPI_Send(&tipoProceso, 1, MPI_INT, i, etiqueta_notificacion_tipo, MPI_COMM_WORLD);
            	printf("\n0%d) %d", i, tipoProceso); 
            	i++;
        	}
    	}
    
    	key = malloc(sizeof(char) * tamPalabra);
    	key = genera_palabra(tamPalabra); 
    
    	//Envio de la palabra generada a los comprobadores
    
    	//printf("\n");
    	printf("\n\nNOTIFICACION PALABRA COMPROBADORES");
    	for (i = 1; i <= numcomprobadores; i++) 
    	{
        	tipoProceso=0;        	
        	MPI_Send(key, tamPalabra, MPI_CHAR, i, etiqueta_palabra, MPI_COMM_WORLD);
        	printf("\n0%d) %s, %d", i, key, tamPalabra); 
    	}
    	//printf("\n");
		printf("\n\nBUSCANDO\n");          
	}//fin Proceso Entrada/Salida
	
	//Envio de la longitud de la palabra 

	MPI_Bcast(&tamPalabra, 1, MPI_INT, 0, MPI_COMM_WORLD); 

	//Recepcion de los datos enviados por el proceso de E/S

	if (myrank != 0) 
	{
    	MPI_Recv(&recibido1, 1, MPI_INT, 0, etiqueta_notificacion_tipo, MPI_COMM_WORLD, &statusmensaje);
    	if (recibido1 == 0)
    	{
        	key = malloc(sizeof(char) * tamPalabra);
        	MPI_Recv(key, tamPalabra, MPI_CHAR, 0, etiqueta_palabra, MPI_COMM_WORLD, &statusmensaje);
   		}               
	}
	
	tinicial = tfinal = (double) clock();
	
	//Codigo de los procesos generadores

	if (recibido1 != 0 && myrank != 0) { 
		int numero_caracteres_encontrado = 0;
		int contador, fin_busqueda = 1;
		int num_interacciones = 0;
		double tinicial_generar, tfinal_generar, tinicial_espera_comprobar, tfinal_espera_comprobar;

		char * palabra_generada;
		char * palabra_parcialmente_descubierta;
		char * buffer;
		
		MPI_Datatype MPI_STRUCT_GENERADOR;
		estadisticas_generador info;
		double tiempo_generar = 0;
		info.num_iteraciones = 0;
		info.tiempo_generar = 0;
		info.tiempo_espera_comprobar = 0;
		
		tinicial = tfinal = (double) clock();
		
		construir_tipo_derivado_generar(&info, &MPI_STRUCT_GENERADOR);
	
		palabra_parcialmente_descubierta = malloc(sizeof(char) * tamPalabra);
		palabra_generada = malloc(sizeof(char) * tamPalabra);
		buffer = malloc(sizeof(char) * tamPalabra);
		
		palabra_generada = genera_palabra(tamPalabra);
	
		while (fin_busqueda != 0) {
			buffer[0] = '\0';
	        fuerza_espera(PESO_GENERAR);
	        
	        tfinal_generar = (double) clock();
			tiempo_generar = tiempo_generar + (tfinal_generar - tinicial_generar) / (double) CLOCKS_PER_SEC;
			
			tinicial_espera_comprobar = tfinal_espera_comprobar = (double) clock();
			MPI_Isend(palabra_generada, tamPalabra, MPI_CHAR, recibido1, etiqueta_palabra, MPI_COMM_WORLD, &peticion1); 
	
		
			MPI_Recv(palabra_parcialmente_descubierta, tamPalabra, MPI_CHAR, MPI_ANY_SOURCE, etiqueta_palabra, MPI_COMM_WORLD, &statusmensaje); 
			
			tfinal_espera_comprobar = (double) clock();
			info.tiempo_espera_comprobar = info.tiempo_espera_comprobar + (tfinal_espera_comprobar - tinicial_espera_comprobar) / (double) CLOCKS_PER_SEC;
       	
			if (palabra_parcialmente_descubierta[0] != '\0') {
				contador = 0;		
				for (int n = 0; n < tamPalabra-1; n++) {
					if (palabra_parcialmente_descubierta[n] != CHAR_NF) {
						contador = contador + 1;
					}
				}
									
				MPI_Irecv(buffer, tamPalabra, MPI_CHAR, 0, etiqueta_pista, MPI_COMM_WORLD, &peticion2);
				info.num_iteraciones++;

				if (buffer[0] != '\0') {
					for (int i = 0; i < tamPalabra-1; i++) {
						if (buffer[i] != CHAR_NF && palabra_parcialmente_descubierta[i] == CHAR_NF) {
							palabra_parcialmente_descubierta[i] = buffer[i];
							numero_caracteres_encontrado++;
						}
					}
				}
				else {
					if (contador > numero_caracteres_encontrado) {
						MPI_Isend(palabra_parcialmente_descubierta, tamPalabra, MPI_CHAR, 0, etiqueta_palabra, MPI_COMM_WORLD, &peticion2);
						numero_caracteres_encontrado = contador;
					}
				}
				tinicial_generar = tfinal_generar = (double) clock();
				palabra_generada = genera_palabra_con_pista(tamPalabra, palabra_parcialmente_descubierta);
        		}
        	else {
        		fin_busqueda = 0;
        	}
		}
		tfinal = (double) clock();
		
		info.tiempo_total = (tfinal - tinicial) / (double) CLOCKS_PER_SEC;
		info.tiempo_generar = tiempo_generar;

		MPI_Isend(&info, 1, MPI_STRUCT_GENERADOR, 0, etiqueta_estadisticas_generador, MPI_COMM_WORLD, &peticion1);
	}
	
	//Codigo de los procesos comprobadores

	if (recibido1 == 0 && myrank != 0) { 
		int fin_comprobacion = 1;
		char * palabra_recibida;
		int num_interacciones = 0;
		double tinicial_comprobar, tfinal_comprobar;
		
		MPI_Datatype MPI_STRUCT_COMPROBADOR;
		estadisticas_comprobador info;
		info.num_iteraciones = 0;
		info.tiempo_comprobar = 0;
		
		construir_tipo_derivado_comprobar(&info, &MPI_STRUCT_COMPROBADOR);
	
		palabra_recibida = malloc(sizeof(char) * tamPalabra);
		
		tinicial = tfinal = (double) clock();

		while (fin_comprobacion != 0) {
			MPI_Recv(palabra_recibida, tamPalabra, MPI_CHAR, MPI_ANY_SOURCE, etiqueta_palabra, MPI_COMM_WORLD, &statusmensaje); 
							
			if (palabra_recibida[0] != '\0') {
				tinicial_comprobar = tfinal_comprobar = (double) clock();
				
				for (int n = 0; n < tamPalabra-1; n++) {
					if (palabra_recibida[n] != key[n]) {
						palabra_recibida[n] = CHAR_NF;
					}
				}
		
				fuerza_espera(PESO_COMPROBAR);
		
				MPI_Isend(palabra_recibida, tamPalabra, MPI_CHAR, statusmensaje.MPI_SOURCE, etiqueta_palabra, MPI_COMM_WORLD, &peticion1); 
				tfinal_comprobar = (double) clock();
				info.tiempo_comprobar = info.tiempo_comprobar + ((tfinal_comprobar - tinicial_comprobar) / (double) CLOCKS_PER_SEC);

				info.num_iteraciones++;
			}
			else {
				fin_comprobacion = 0;
			}
		}
		tfinal = (double) clock();
		info.tiempo_total = (tfinal - tinicial) / (double) CLOCKS_PER_SEC;

		MPI_Isend(&info, 1, MPI_STRUCT_COMPROBADOR, 0, etiqueta_estadisticas_comprobador, MPI_COMM_WORLD, &peticion1);
	}
	
	//Codigo del proceso de E/S

	if (myrank == 0) { 
		int acertado = 0;
		char * palabra_parcialmente_descubierta;

		palabra_parcialmente_descubierta = malloc(sizeof(char) * tamPalabra);
		do {
			MPI_Recv(palabra_parcialmente_descubierta, tamPalabra, MPI_CHAR, MPI_ANY_SOURCE, etiqueta_palabra, MPI_COMM_WORLD, &statusmensaje);
	
			printf("El proceso %d envia la siguiente palabra: %s\n", statusmensaje.MPI_SOURCE, palabra_parcialmente_descubierta);
			fflush(stdout);
			if (atoi(argv[2]) == 1) {
				for (int n = numcomprobadores + 1; n < nprocs; n++) {
					if (n != statusmensaje.MPI_SOURCE) {
						MPI_Send(palabra_parcialmente_descubierta, tamPalabra, MPI_CHAR, n, etiqueta_pista, MPI_COMM_WORLD); 
					}
				}
			}
	
			acertado = 0;
			for (int n = 0; n < tamPalabra-1; n++) {
				if (palabra_parcialmente_descubierta[n] == CHAR_NF) {
					acertado = 1;
					break;
				}
			}
	
			if (acertado == 0) {
				//fflush(stdout);
				printf("\nPALABRA ENCONTRADA POR %d", statusmensaje.MPI_SOURCE);
				printf("\nBUSCADA...: %s", key);
				printf("\nENCONTRADA: %s", palabra_parcialmente_descubierta);
				palabra_parcialmente_descubierta[0] = '\0';
				for (int n = 1; n < nprocs; n++) {
					MPI_Send(palabra_parcialmente_descubierta, tamPalabra, MPI_CHAR, n, etiqueta_palabra, MPI_COMM_WORLD);
				}
			}		
		}	
		while (acertado != 0);
		
		//Recepcion de las estadisticas enviadas por los comprobadores y los generadores
		
		int acumulador_iteraciones_comprobador = 0;
		int acumulador_iteraciones_generador = 0;
		estadisticas_comprobador info_comprobador;
		estadisticas_generador info_generador;
		
		MPI_Datatype MPI_STRUCT_COMPROBADOR;
		MPI_Datatype MPI_STRUCT_GENERADOR;
		
		construir_tipo_derivado_comprobar(&info_comprobador, &MPI_STRUCT_COMPROBADOR);
		construir_tipo_derivado_generar(&info_generador, &MPI_STRUCT_GENERADOR);
		
		printf("\n\n>-------------------------------COMPROBADORES--------------------------------<");
		printf("\n    Iteraciones  |  TotalIteraciones  |  TiempoTotal  |    TiempoComprobacion");			
		for (int i = 1; i <= numcomprobadores; i++) {
			MPI_Recv(&info_comprobador, 1, MPI_STRUCT_COMPROBADOR, i, etiqueta_estadisticas_comprobador, MPI_COMM_WORLD, &statusmensaje);
			acumulador_iteraciones_comprobador = acumulador_iteraciones_comprobador + info_comprobador.num_iteraciones;
			printf("\n0%d)   %5d      |      %5d         | %11.6f   | %11.6f (%.6f)", statusmensaje.MPI_SOURCE, info_comprobador.num_iteraciones, acumulador_iteraciones_comprobador, info_comprobador.tiempo_total, info_comprobador.tiempo_comprobar, info_comprobador.tiempo_comprobar / info_comprobador.tiempo_total);			
		}	
		
		printf("\n\n>-------------------------------------------------GENERADORES-------------------------------------------------<");
		printf("\n    Iteraciones  |  TotalIteraciones  |  TiempoTotal  |      TiempoGeneracion      |  TiempoEsperaComprobacion");
		for (int i = numcomprobadores+1; i < nprocs; i++) {
			MPI_Recv(&info_generador, 1, MPI_STRUCT_GENERADOR, i, etiqueta_estadisticas_generador, MPI_COMM_WORLD, &statusmensaje);
			acumulador_iteraciones_generador = acumulador_iteraciones_generador + info_generador.num_iteraciones;
			printf("\n0%d)   %5d      |      %5d         | %11.6f   |  %11.6f (%.6f)    |  %11.6f (%.6f)", statusmensaje.MPI_SOURCE, info_generador.num_iteraciones, acumulador_iteraciones_generador, info_generador.tiempo_total, info_generador.tiempo_generar, info_generador.tiempo_generar / info_generador.tiempo_total, info_generador.tiempo_espera_comprobar, info_generador.tiempo_espera_comprobar / info_generador.tiempo_total);
		}
	}

	tfinal = (double) clock();
	
	//printf("\n");
	if (myrank == 0) {
		printf("\n\n>---------ESTADISTICAS TOTALES----------<");
		printf("\nNumero Procesos: %d \n", nprocs);
		printf("Tiempo procesamiento: %f \n", (tfinal - tinicial) / (double) CLOCKS_PER_SEC);
		printf("\n>---------FIN----------<\n");
	}

	MPI_Finalize();

	return 0;
}


char * genera_palabra(int tamPalabra)
{
	srand(time(NULL) * (double) clock()); 
	char * key = malloc(sizeof(char) * tamPalabra); 
	int i;
		
	for (i = 0; i < tamPalabra; i++) {
		key[i] = (char)(rand() % (CHAR_MAX + 1 - CHAR_MIN)) + CHAR_MIN;
	}
	key[i-1] = '\0';
    return key;
}

char * genera_palabra_con_pista(int tamPalabra,char *vectorPista)
{
	srand(time(NULL) * (double) clock()); 
	char * key = malloc(sizeof(char) * tamPalabra); 
	int i;
	
	for (i = 0; i < tamPalabra; i++) {
		if (vectorPista[i] == CHAR_NF) {
			key[i] = (char)(rand() % (CHAR_MAX + 1 - CHAR_MIN)) + CHAR_MIN;
		}
		else {
			key[i] = vectorPista[i];
		}
	}
	key[i-1] = '\0';
	return key;
}

void fuerza_espera(unsigned long peso)
{
	for (unsigned long i=1; i<1*peso; i++) sqrt(i);
}

void construir_tipo_derivado_generar(estadisticas_generador * pdatos, MPI_Datatype * pMPI_Tipo_Datos) {
	MPI_Datatype tipos[4];
	int longitudes[4];
	MPI_Aint direcc[5];
	MPI_Aint desplaz[4];

	/*PRIMERO ESPECIFICAMOS LOS TIPOS*/
	tipos[0] = MPI_INT;
	tipos[1] = MPI_DOUBLE;
	tipos[2] = MPI_DOUBLE;
	tipos[3] = MPI_DOUBLE;

	/*ESPECIFICAMOS EL NUMERO DE ELEMENTOS DE CADA TIPO*/
	longitudes[0]=1;
	longitudes[1]=1;
	longitudes[2]=1;
	longitudes[3]=1;

	/*CALCULAMOS LOS DESPLAZAMIENTOS DE LOS MIEMBROS DE LA ESTRUCTURA.
	RELATIVOS AL COMIENZO DE DICHA ESTRUCTURA*/
	MPI_Address(pdatos, &direcc[0]);
	MPI_Address(&(pdatos->num_iteraciones), &direcc[1]);
	MPI_Address(&(pdatos->tiempo_total), &direcc[2]);
	MPI_Address(&(pdatos->tiempo_generar), &direcc[3]);
	MPI_Address(&(pdatos->tiempo_espera_comprobar), &direcc[4]);
	desplaz[0]=direcc[1]-direcc[0];
	desplaz[1]=direcc[2]-direcc[0];
	desplaz[2]=direcc[3]-direcc[0];
	desplaz[3]=direcc[4]-direcc[0]; 

	/*CREACION TIPO DATOS DERIVADO*/
	MPI_Type_struct(4, longitudes, desplaz, tipos, pMPI_Tipo_Datos);

	/*CERTIFICARLO DE MANERA QUE PUEDA SER USADO*/

	MPI_Type_commit(pMPI_Tipo_Datos);
} /*construir_tipo_derivado*/

void construir_tipo_derivado_comprobar(estadisticas_comprobador * pdatos, MPI_Datatype * pMPI_Tipo_Datos) {
	MPI_Datatype tipos[3];
	int longitudes[3];
	MPI_Aint direcc[4];
	MPI_Aint desplaz[3];

	/*PRIMERO ESPECIFICAMOS LOS TIPOS*/
	tipos[0] = MPI_INT;
	tipos[1] = MPI_DOUBLE;
	tipos[2] = MPI_DOUBLE;

	/*ESPECIFICAMOS EL NUMERO DE ELEMENTOS DE CADA TIPO*/
	longitudes[0]=1;
	longitudes[1]=1;
	longitudes[2]=1;

	/*CALCULAMOS LOS DESPLAZAMIENTOS DE LOS MIEMBROS DE LA ESTRUCTURA.
	RELATIVOS AL COMIENZO DE DICHA ESTRUCTURA*/
	MPI_Address(pdatos, &direcc[0]);
	MPI_Address(&(pdatos->num_iteraciones), &direcc[1]);
	MPI_Address(&(pdatos->tiempo_total), &direcc[2]);
	MPI_Address(&(pdatos->tiempo_comprobar), &direcc[3]);
	desplaz[0]=direcc[1]-direcc[0];
	desplaz[1]=direcc[2]-direcc[0];
	desplaz[2]=direcc[3]-direcc[0];

	/*CREACION TIPO DATOS DERIVADO*/
	MPI_Type_struct(3, longitudes, desplaz, tipos, pMPI_Tipo_Datos);

	/*CERTIFICARLO DE MANERA QUE PUEDA SER USADO*/

	MPI_Type_commit(pMPI_Tipo_Datos);
} /*construir_tipo_derivado*/

