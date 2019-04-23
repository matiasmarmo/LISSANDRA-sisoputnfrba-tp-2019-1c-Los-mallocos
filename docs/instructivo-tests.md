# Testing 
- `int main(int argc, char* argv[]);`

La estructura de los tests esta hecha de forma tal que al darle Run al `main` de test sea necesario pasarle qué bloque (o bloques) de Tests voy a ejecutar. 
Esto es un beneficio dado que si yo solo quiero probar la funcionalidad del modulo XXX, solamente debo ejecutar en la consola lo siguiente:
```sh
$ ./test.out   XXX
```
De igual manera, si quiero probar 2 o mas modulos se debe ejecutar la siguiente linea:
```sh
$ ./test.out   XXX   YYY   ZZZ  (etc...)
```
Aclaracion: Los bloques de tests estan previamente definidos con un nombre distintivo cada uno. Por ejemplo...
```sh
$ ./test.out   Parser   Consola 
```
Por ultimo, existe una funcion "All" la cual ejecuta absolutamente todos los test del proyecto:
```sh
$ ./test.out   All    
```