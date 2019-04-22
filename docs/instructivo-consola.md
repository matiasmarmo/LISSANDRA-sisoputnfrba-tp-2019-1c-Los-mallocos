# Consola
***
La consola expone las siguientes funcionalidades:
 - Instanciar una consola para poder realizar peticiones,
 - Convertir mensajes del protocolo de comunicacion en strings y
 - Convertir mensajes del protocolo de comunicacion en strings y mostrarlos por pantalla.
 
### Uso de la API
##### 1. Para instanciar una consola y realizar peticiones
- **void iniciar_consola(handler_t handler);**
Inicia una consola lista para recibir peticiones. Le pasaremos un puntero a una funcion de tipo handler_t que recibira un mensaje del protocolo y realizara el trabajo que se le haya definido. La firma de esta funcion es: typedef void (\*handler_t)(void\*);.

- **void leer_siguiente_caracter();**
Permite leer un caracter desde la consola. Se manejara la linea ingresada cuando detecte fin de linea **'/n'**. Cuando esto suceda, se crea un mensaje del protocolo en base a la peticion ingresada y se manejara dicho mensaje con la funcion de tipo handler_t que le pasamos a la funcion anterior.

- **void cerrar_consola();**
Cierra la consola y libera la memoria que haya usado.

##### 2. Para convertir mensajes del protocolo en strings
- **void iniciar_diccionario();**
Crea un diccionario de funciones que convierten un mensaje del protocolo en un string apropiado a la estructura del mensaje.

- **int convertir_en_string(void\* mensaje, char\* buffer, int tamanio_maximo);**
Recibe un mensaje del protocolo, un buffer y el tamanio del buffer. El mensaje es descompuesto y con él se arma un string que se guarda en el buffer.

- **void cerrar_diccionario();**
Elimina el diccionario y libera la memoria que se haya usado.

##### 3. Para convertir mensajes del protocolo en strings y mostrarlos por pantalla
- **void mostrar(void\* mensaje);**
Igual que convertir_en_string(3) pero uncamente recibe el mensaje (internamente, el tamanio maximo del string esta definido en 400). Convierte en string el mensaje y lo muestra por pantalla.
_Importante: Tambien es necesario crear el diccionario (y eliminarlo)_
