# Búsqueda de una palabra secreta con programación paralela MPI

![Inicio](https://github.com/rmelgo/ARQ-Busqueda-palabra-secreta-MPI/assets/145989723/1eb7469c-9e5d-46d8-80bc-f58a93e2b035)

# - Introducción

Proyecto realizado en la asignatura de Arquitectura de Computadores del grado de Ingenieria Informática de la Universidad de Salamanca. El enunciado del proyecto se encuentra subido en el repositorio en un archivo PDF llamado *EnunciadoPrácticaMP2122I.pdf*.
  
El principal objetivo de este proyecto es la realización de un programa en C que explote la paralelización de una tarea a traves de una interfaz de paso de mensajes entre procesos llamada **MPI**.
En este caso, se pretende diseñar un programa que permita encontrar una palabra “secreta” de una longitud determinada generando combinaciones aleatorias, de manera que, se comprueben, evitando que los caracteres acertados no se vuelvan a generar.
Para ello, existirán 3 tipos de procesos (E/S, Comprobadores y Generadores) donde cada uno de los procesos tendrá un rol diferente.

Otro objetivo que se persigue es evaluar el redimiento de la prática, es decir, el tiempo empleado para descubrir la palabra secreta en función de la longitud de la palabra y el número de procesos generadores y comprobadores.

# - Comentarios sobre el entorno de ejecución

Para ejecutar este programa, se requerira de una distribución del Sistema Operativo **GNU/Linux**.    

Para poder compilar correctamente el programa, se deberá tener instalada una version del compilador mpicc. En el caso de no tener mpicc, se puede instalar facilmente con el siguiente comando:

```sudo apt install libopenmpi-dev```

# Comentarios sobre el material adjuntado

El proyecto cuenta con los siguientes ficheros:

- Un fichero llamado ***mpi.c*** que contiene todo el código necesario para el correcto funcionamiento de la practica.
- Un documento llamado ***TRABAJO_MPI.pptx*** que contiene una presentación en PowerPoint en la que se explican los aspectos más importantes del desarrollo del proyecto (estructura, parámetros, aspectos mas relevantes de la implementación) junto con unas ejecuciones de prueba y un análisis del resultado de dichas pruebas.

# - Parámetros de ejecución

Para ejecutar el programa es necesario proporcionar 3 argumentos. 

- El primer argumento hace referencia al **número total de procesos** que se crearán al ejecutar el programa. Cada proceso se ejecutará de manera paralela en un nucleo del procesador. El proceso padre o proceso raiz siempre será el proceso de E/S.

- El segundo argumento hace referencia al **número de procesos Comprobadores** que se crearán al ejecutar el programa. Para ejecutar correctamente el programa es necesario que al menos exista un proceso Comprobador.

- El tercer argumento hace referencia al **modo pista** donde un 1 indica que el modo pista esta activado y un 0 indica que el modo pista esta desactivado. Mas adelante, se explicará en que consiste el modo pista.

**Nota**: Aunque el número de procesos Generadores no se indique como parámetro se puede obtener realizando el siguiente calculo:    

```<número total de procesos> - <número de procesos Comprobadores> - 1```

Para ejecutar correctamente el programa debe existir al menos:

- Un *proceso de E/S*
- Un *proceso Comprobador*
- Un *proceso Generador*

De esta manera, el **número total de procesos** debe ser 2 unidades mayor que el **número de procesos Comprobadores**.

Si los parámetros introducidos no respetan las reglas anteriores, el programa lo detectará, informará al usuario y acabará.

# Estructura y funcionamiento del proyecto

## Tipos de procesos

El objetivo del proyeccto es diseñar un programa que permita encontrar un palabra "secreta". Para ello, van a existir 3 tipos de procesos diferentes:

- **Proceso de E/S**: Este proceso se encarga de las siguientes tareas:

  - Comprobar que los argumentos introducidos son correctos.
  - Generar la palabra secreta de manera totalmente aleatoria.
  - Notificar al resto de proceso la longitud en caracteres de la palabra secreta.
  - Notificar a los *procesos Comprobadores* el contenido de la palabra secreta.
  - Notificar a cada proceso su rol, es decir, si es un *proceso Generador* o un *proceso Comprobador*.
  - Asignar a cada *proceso Generador* un *proceso Comprobador*.
  - Mantenerse a la espera de recibir de los *procesos Generadores* una cadena con los carácteres de la palabra secreta ya encontrados. Si el modo pista esta activado, esta cadena se distribuirá al resto de *procesos Generadores*.
  - Cuando reciba de los *procesos Generadores* la palabra ya encontrada, mandará terminar a los *procesos Generadores* y a los *procesos Comprobadores* y recibirá las estadísticas por parte de los *procesos Generadores* y los *procesos Comprobadores*.

- **Procesos Comprobadores**: Estos procesos se encargan de las siguientes tareas:

  - Recibir la logitud de la palabra por parte del *proceso de E/S*.
  - Recibir una indicación por parte del *proceso de E/S* del tipo de rol que son.
  - Recibir la palabra secreta que se debe comprobar.
  - Mantenerse a la espera de recibir de los *procesos Generadores* una palabra.
  - Cuando reciban una palabra de un *proceso Generador*, se deberá comprobar la palabra y devolver al *proceso Generador* una respuesta con los carácteres de la palabra secreta que han sido acertados.
  - Cuando el *proceso de E/S* mande la finalización del proceso, deberan enviar las estadísticas recogidas en el proceso (tiempo y número de comprobaciones) al *proceso de E/S*.

- **Procesos Generadores**: Estos procesos se encargan de las siguientes tareas:

  - Recibir la logitud de la palabra por parte del *proceso de E/S*.
  - Recibir una indicación por parte del *proceso de E/S* del tipo de rol que son.
  - Recibir el *proceso Comprobador* asociado.
  - Generar palabras aleatorias de la longitud indicada, teniendo en cuenta los caracteres de la palabra secreta ya acertados.
  - Enviar las palabras generadas al *proceso Comprobador* asociado.
  - Recibir del *proceso Comprobador* asociado una respuesta y actualizar el conjunto de caracteres de la palabra secreta ya acertados.
  - Si el modo pista esta activado, recibir del *proceso de E/S* una cadena con los carácteres de la palabra secreta ya encontrados por otro *proceso Generador*.
  - Cuando el *proceso de E/S* mande la finalización del proceso, deberan enviar las estadísticas recogidas en el proceso (número de comprobaciones, tiempo de generación y tiempo de espera de comprobación) al *proceso de E/S*.

## Funcionamiento

Una vez explicadas las tareas que realizan los disintos procesos, se explicará el funcionamiento del proyecto cuando se ejecuta a traves de la terminal. En la ejecución del programa se producen los siguientes pasos:

- **Paso 1**: El *proceso de E/S* comprueba que los parámetros introducidos en la ejecución del programa son correctos. En el caso de no ser correctos, el *proceso de E/S* finaliza la ejecución del programa.
- **Paso 2**: El *proceso de E/S* imprime por pantalla el número de procesos de cada tipo.
- **Paso 3**: El *proceso de E/S* notifica a cada proceso su rol. A los *procesos Comprobadores* envia un 0 y a los *procesos Generadores* envia el número del *proceso Comprobador* asociado.
- **Paso 4**: El *proceso de E/S* envia la longitud de la palabra secreta a todos los procesos y la palabra secreta a los *procesos Comprobadores*.
- **Paso 5**: Los *procesos Generadores* generan palabras aleatorias y las envian a sus *procesos Comprobadores* asociados. Estos comprueban las palabras recibidas y devuelven una cadena con los carácteres de la palabra secreta ya encontrados. Cada vez que reciben una respuesta del *proceso Comprobador*, los *procesos Generadores* actualizan su cadena con los carácteres de la palabra secreta ya encontrados.
- **Paso 6**: Cada vez que un *proceso Generador* encuentre uno (o varios) caracteres de la palabra secreta, enviara al *proceso de E/S* una cadena con los carácteres de la palabra secreta ya encontrados por el *proceso Generador*. De esta manera, el *proceso de E/S* imprimirá por pantalla dicha cadena con los carácteres de la palabra secreta ya encontrados. Si el modo pista esta activado, el *proceso de E/S* distribuirá dicha cadena al resto de *procesos Generadores* facilitando la busqueda de la palabra secreta.
- **Paso 7**: Cuando un *proceso Generador* encuentre la palabra secreta, el *proceso de E/S* imprimirá la palabra secreta por la pantalla y el *proceso Generador* que la ha encontrado. De esta manera, el *proceso de E/S* mandará terminar al resto de procesos.
- **Paso 8**: Tanto los *procesos Generadores* como los *procesos Comprobadores* enviar las estadísticas recogidas al *proceso de E/S* y este las imprimirá por pantalla. 

Las estadisticas de los *procesos Generadores* incluyen:

- Tiempo total empleado en la ejecución
- Tiempo empleado en la generación
- Tiempo empleado en la espera de la comprobación
- Número de generaciones realizada

Las estadisticas de los *procesos Comprobadores* incluyen:

- Tiempo total empleado en la ejecución
- Tiempo empleado en la comprobación
- Número de comprobaciones realizada

# - Pasos necesarios para ejecutar el programa

**Paso 1: Compilar el programa**  

Para ello se debe introducir el siguiente comando:    

```mpicc mpi.c -o mpi -lm```

Tras ejecutar este comando, se generará un fichero ejecutable llamado *mpi*. 

**Paso 2: Ejecutar el programa**  

Para ello se debe introducir el siguiente comando:    

```mpirun -np <número-procesos> mpi <número-comprobadores> <modo-pista>```

Tras ejecutar este comando, el programa se habra ejecutado correctamente, siempre y cuendo se hayan introducido los argumentos correspondientes.

# - Ejemplos de ejecución

## Ejecución con 3 procesos, 1 comprobador, 1 generador y modo pista activado

En la siguiente imagen, se muestra un ejemplo del uso y funcionamiento del programa con 1 *proceso Comprobador*, 1 *proceso Generador* y el modo pista activado:    

![Ejemplo ejecucion 1](https://github.com/rmelgo/ARQ-Busqueda-palabra-secreta-MPI/assets/145989723/c1e8fc87-8866-4d18-815f-a1405affd83d)
![Ejemplo ejecucion 2](https://github.com/rmelgo/ARQ-Busqueda-palabra-secreta-MPI/assets/145989723/c102fac6-abc1-4b7e-bb09-4973a4ce5703)

## Ejecución con 3 procesos, 1 comprobador, 1 generador y modo pista desactivado

En la siguiente imagen, se muestra un ejemplo del uso y funcionamiento del programa con 1 *proceso Comprobador*, 1 *proceso Generador* y el modo pista desactivado:    

![Ejemplo ejecucion 3](https://github.com/rmelgo/ARQ-Busqueda-palabra-secreta-MPI/assets/145989723/c6770f7c-f8fc-44a9-8eac-d06491666bb5)
![Ejemplo ejecucion 4](https://github.com/rmelgo/ARQ-Busqueda-palabra-secreta-MPI/assets/145989723/1def15fd-024e-41cd-9cd0-64122aee47af)

## Ejecución con 5 procesos, 2 comprobadores, 2 generadores y modo pista activado

En la siguiente imagen, se muestra un ejemplo del uso y funcionamiento del programa con 2 *procesos Comprobadores*, 2 *procesos Generadores* y el modo pista activado:   

![Ejemplo ejecucion 5](https://github.com/rmelgo/ARQ-Busqueda-palabra-secreta-MPI/assets/145989723/f4c39c1c-7d2e-467b-a5f0-711b4dd0ab7f)
![Ejemplo ejecucion 6](https://github.com/rmelgo/ARQ-Busqueda-palabra-secreta-MPI/assets/145989723/bc359dfe-bdec-4d88-b6a1-0bbb7a1db5d2)

## Ejecución con 5 procesos, 2 comprobadores, 2 generadores y modo pista desactivado

En la siguiente imagen, se muestra un ejemplo del uso y funcionamiento del programa con 2 *procesos Comprobadores*, 2 *procesos Generadores* y el modo pista desactivado:  

![Ejemplo ejecucion 7](https://github.com/rmelgo/ARQ-Busqueda-palabra-secreta-MPI/assets/145989723/c5df936a-082d-4847-9746-8ecf2418d513)
![Ejemplo ejecucion 8](https://github.com/rmelgo/ARQ-Busqueda-palabra-secreta-MPI/assets/145989723/6e442b4d-1ff3-4a12-a8a6-0115a28acacf)
![Ejemplo ejecucion 9](https://github.com/rmelgo/ARQ-Busqueda-palabra-secreta-MPI/assets/145989723/7cc83a67-858b-467d-8d18-9c5bd0de2513)

## Conclusiones 

- El tiempo empleado en descubrir la palabra secreta disminuye a medida que se aumenta el numero de *procesos Comprobadores* y *procesos Generadores*.
- El modo pista disminuye el tiempo empleado en descubrir la palabra secreta siempre y cuando haya mas de un *proceso Generador*.
