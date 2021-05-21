#include "i-mongo-store-lib.h"

int main(int argc, char*argv[])
{
	//Al levantar el modulo, primer argumento: path del archivo de config
	//segundo argumento: path donde quiero crear el log del modulo
	i_mongo_store_configuracion = leer_config_i_mongo_store(argv[1]);
	logger_i_mongo_store = iniciar_logger_i_mongo_store(argv[2]);
	inicializar_rutas(i_mongo_store_configuracion->PUNTO_MONTAJE);

	inicializar_filesystem(10,10);
	esperar_conexion(i_mongo_store_configuracion->PUERTO); 
}

void inicializar_rutas(char* montaje){
	ruta_superbloque = malloc(strlen(montaje)+strlen("/Superbloque.ims")+1);
	memcpy(ruta_superbloque,montaje,strlen(montaje)+1);
	strcat(ruta_superbloque,"/Superbloque.ims");
	ruta_blocks = malloc(strlen(montaje)+strlen("/Blocks.ims")+1);
	memcpy(ruta_blocks,montaje,strlen(montaje)+1);
	strcat(ruta_blocks,"/Blocks.ims");
	carpeta_files = malloc(strlen(montaje)+strlen("/Files/")+1);
	memcpy(carpeta_files,montaje,strlen(montaje)+1);
	strcat(carpeta_files,"/Files/");
	carpeta_bitacoras = malloc(strlen(montaje)+strlen("/Bitacoras/")+1);
	memcpy(carpeta_bitacoras,montaje,strlen(montaje)+1);
	strcat(carpeta_bitacoras, "/Bitacoras/");
}