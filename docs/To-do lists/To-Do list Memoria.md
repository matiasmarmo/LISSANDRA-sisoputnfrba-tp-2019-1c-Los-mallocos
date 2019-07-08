# To-Do list Memoria!

- [ ] Hacer que la parte del checkpoint de errores ande
- [x] Ni bien inicializa la memoria pedirle al FS el 
      maximo tamaño del value (Handshake)
- [x] Ajustar el bit de modificado
- [x] Hacer funcion que tome los datos de una pagina y 
      deje esa porcion de memoria en '0'
- [x] Hacer el Gossiping
- [x] Hacer error de que si la memoria no puede inicializarse 
      o si no puede hacer el Handshake aborte el proceso e 
      informe el problema
- [x] Hacer el algoritmo LRU 
- [x] Hacer los logs del programa
- [x] Pedir y recibir pagina del FS
- [x] Terminar el SELECT
- [x] Terminar el INSERT
- [x] Hacer el CREATE
- [x] Hacer el DESCRIBE
- [x] Hacer el DROP
- [x] Hacer el Journaling cuando memoria este Full o cuando 
      recibe por consola o por el kernel la solicitud o 
      cuando pasa X tiempo
	- [ ] Hacer que cuando memoria este en Journal no 
              responda pedidos que requiera el reemplazo de 
              uno de los segmentos actuales
	- [ ] Hacer que cuando termine el Journal vuelva a 
              aceptar pedidos
	- [ ] Cuando una memoria se encuentra procesando o 
              realizando un Journal a Disco no se podrá operar 
              ningún request que le llegue a la memoria, 
              debiendo esperar a que el primero finalice.
- [x] Probar el journaling 
- [ ] Hacer retardos


Colocando una x entre los corchetes se pone como marcado