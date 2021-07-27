# tp-2021-1c-S-quito-de-Oviedo

<p align="center"><img src="oviedo.jpg" width="200px"/></p>



## COMANDOS DE LA CONSOLA

````powershell
INICIAR_PATOTA 2 plantas.txt 1|1 3|4
LISTAR_TRIPULANTES
INICIAR_PLANIFICACION
PAUSAR_PLANIFICACION
EXPULSAR_TRIPULANTE 3
OBTENER_BITACORA 1
````



## I-MONGO-STORE

Tenemos los siguientes comandos

- `./execb`    :    remueve la carpeta **polus** e inicia i-mongo
-  `./exec`    :    Inicia i-mongo




#### Ver logs filtrados
Ubicarse en el `/cfg` antes de ejecutar el comando
````powershell
cat discordiador.log | grep "Tripulante 1"
````



## Integrantes

| Integrante | Github | Legajo | Correo | Curso
|--|--|--|--|--
| **Gianpier Yupanqui** | [@gianpieryup](https://www.github.com/gianpieryup) | 159.207-5 | gianpieryup@gmail.com | K3054
| **Damian Teplitz** | [@dteplitz](https://www.github.com/dteplitz) | 158.780-8 | dteplitz@frba.utn.edu.ar | K3154
| **Santiago Feijoo** | [@SantiagoIvan](https://github.com/SantiagoIvan) | 152.288-7 | santiago.feijoo96@gmail.com | K3153
| **Cristian Cali** | [@julchat](https://www.github.com/julchat) | 167.318-0 | crcali@est.frba.utn.edu.ar | K3052
| **Juan Manuel Castagno** | [@jcastagno99](https://www.github.com/jcastagno99) | 167.863-2 | Juan-Castagno@Hotmail.com | K3054


## Manejo de señales

La forma de enviar señales a un proceso es con el comando:
````powershell
kill -SIGNAL <pid>
````

Para mandar una señal a mi-ram-hq debemos saber el pid que posee
Uso pgrep <nombrePrograma>
````powershell
pgrep mi-ram-hq
````


Con ese pid tiro cualquiera de las 2 señales configuradas, SIGUSR1 y SIGUSR2 .
````powershell
kill -SIGUSR1 <pid> o kill -SIGUSR2
````


SIGUSR1 va a generar la compactacion
SIGUSR2 va a loggear toda la informacion en memoria actualmente. 

Esta no fue pedida pero la desarrollamos pensando en su utilidad para saber que esta pasando en cierto momento en memoria. 