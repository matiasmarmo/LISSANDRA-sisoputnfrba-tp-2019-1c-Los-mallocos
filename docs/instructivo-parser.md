### Mensajes aceptados

| ID | ESTRUCTURA |
| ------ | ------ |
| SELECT | SELECT [NOMBRE TABLA] [KEY] |
| INSERT | INSERT [NOMBRE TABLA] [KEY] "[VALUE]" [TIMESTAMP] -- en caso de ser corrido en el proceso LFS |
| INSERT | INSERT [NOMBRE TABLA] [KEY] "[VALUE]"  -- en caso de ser corrido en los procesos memoria o el kernel |
| CREATE | CREATE [NOMBRE TABLA] [TIPO CONSISTENCIA] [NUMERO PARTICIONES] [TIEMPO COMPACTACION] |
| DESCRIBE | DESCRIBE  ó  DESCRIBE [NOMBRE TABLA] |
| DROP | DROP [NOMBRE TABLA] |
| JOURNAL | JOURNAL [NOMBRE TABLA] |
| ADD MEMORY | ADD MEMORY [NUMERO] TO [CRITERIO] |
| METRICS | METRICS |
| EXIT | EXIT |
| RUN | ARCHIVO |

### Devolucion

| ID | ESTRUCTURA |
| ------ | ------ |
| VALUE_INVALIDO | -7 |
| CONSISTENCIA_INVALIDA | -6 |
| ERROR_TAMANIO_BUFFER | -5 |
| IDENTIFICADOR_INVALIDO | -4 |
| CONSTANTE_INVALIDA | -3 |
| COMANDOS_INVALIDOS | -2 |
| ERROR | -1 |
| OK | 1 |