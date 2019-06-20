# To-Do list Memoria!

- [ ] Hacer que la parte del checkpoint de errores ande
- [ ] Ni bien inicializa la memoria pedirle al FS el 
      maximo tamaño del value (Handshake)
- [ ] Ajustar el bit de modificado
- [ ] Hacer funcion que tome los datos de una pagina y 
      deje esa porcion de memoria en '0'
- [ ] Hacer el Gossiping
- [ ] Hacer error de que si la memoria no puede inicializarse 
      o si no puede hacer el Handshake aborte el proceso e 
      informe el problema
- [ ] Hacer el algoritmo LRU 
- [ ] Hacer los logs del programa
- [ ] Pedir y recibir pagina del FS
- [ ] Terminar el SELECT
- [ ] Terminar el INSERT
- [ ] Hacer el CREATE
- [ ] Hacer el DESCRIBE
- [ ] Hacer el DROP
- [ ] Hacer el Journaling cuando memoria este Full o cuando 
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




Colocando una x entre los corchetes se pone como marcado