#!/usr/bin/python3

from os.path import abspath, dirname
from os import chdir, system, getcwd, mkdir
from sys import argv

IP_FS = "127.0.0.1"
IP_MEMORIA_1 = "127.0.0.1"
IP_MEMORIA_2 = "127.0.0.1"
IP_MEMORIA_3 = "127.0.0.1"
IP_MEMORIA_4 = "127.0.0.1"
IP_MEMORIA_5 = "127.0.0.1"
IP_KERNEL = "127.0.0.1"

configs = {
    "prueba_base": {
        "lfs.conf": {
            "PUERTO_ESCUCHA": 5003,
            "PUNTO_MONTAJE": "/home/utnso/lfs-base/",
            "RETARDO": 0,
            "TAMANIO_VALUE": 60,
            "TIEMPO_DUMP": 20000
        },
        "memoria1.conf": {
            "PUERTO_ESCUCHA": 8001,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [IP_MEMORIA_2],
            "PUERTO_SEEDS": [8002],
            "RETARDO_MEM": 0,
            "RETARDO_FS": 0,
            "TAM_MEM": 1280,
            "RETARDO_JOURNAL": 60000,
            "RETARDO_GOSSIPING": 10000,
            "MEMORY_NUMBER": 1
        },
        "memoria2.conf": {
            "PUERTO_ESCUCHA": 8002,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [],
            "PUERTO_SEEDS": [],
            "RETARDO_MEM": 0,
            "RETARDO_FS": 0,
            "TAM_MEM": 1280,
            "RETARDO_JOURNAL": 60000,
            "RETARDO_GOSSIPING": 10000,
            "MEMORY_NUMBER": 2
        },
        "kernel.conf": {
            "IP_MEMORIA": IP_MEMORIA_1,
            "PUERTO_MEMORIA": 8001,
            "NUMERO_MEMORIA": 1,
            "QUANTUM": 3,
            "MULTIPROCESAMIENTO": 1,
            "METADATA_REFRESH": 15000,
            "MEMORIAS_REFRESH": 5000,
            "SLEEP_EJECUCION": 100
        },
        "Metadata.bin": {
            "BLOCK_SIZE": 64,
            "BLOCKS": 4096,
            "MAGIC_NUMBER": "LISSANDRA"
        }
    },
    "prueba_kernel": {
        "lfs.conf": {
            "PUERTO_ESCUCHA": 5003,
            "PUNTO_MONTAJE": "/home/utnso/lfs-prueba-kernel/",
            "RETARDO": 0,
            "TAMANIO_VALUE": 60,
            "TIEMPO_DUMP": 60000
        },
        "memoria1.conf": {
            "PUERTO_ESCUCHA": 8001,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [IP_MEMORIA_3],
            "PUERTO_SEEDS": [8003],
            "RETARDO_MEM": 1000,
            "RETARDO_FS": 0,
            "TAM_MEM": 4096,
            "RETARDO_JOURNAL": 70000,
            "RETARDO_GOSSIPING": 5000,
            "MEMORY_NUMBER": 1
        },
        "memoria2.conf": {
            "PUERTO_ESCUCHA": 8002,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [IP_MEMORIA_4],
            "PUERTO_SEEDS": [8004],
            "RETARDO_MEM": 1000,
            "RETARDO_FS": 0,
            "TAM_MEM": 2048,
            "RETARDO_JOURNAL": 30000,
            "RETARDO_GOSSIPING": 5000,
            "MEMORY_NUMBER": 2
        },
        "memoria3.conf": {
            "PUERTO_ESCUCHA": 8003,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [],
            "PUERTO_SEEDS": [],
            "RETARDO_MEM": 1000,
            "RETARDO_FS": 0,
            "TAM_MEM": 2048,
            "RETARDO_JOURNAL": 70000,
            "RETARDO_GOSSIPING": 5000,
            "MEMORY_NUMBER": 3
        },
        "memoria4.conf": {
            "PUERTO_ESCUCHA": 8004,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [IP_MEMORIA_1],
            "PUERTO_SEEDS": [8001],
            "RETARDO_MEM": 1000,
            "RETARDO_FS": 0,
            "TAM_MEM": 4096,
            "RETARDO_JOURNAL": 30000,
            "RETARDO_GOSSIPING": 5000,
            "MEMORY_NUMBER": 4
        },
        "kernel.conf": {
            "IP_MEMORIA": IP_MEMORIA_3,
            "PUERTO_MEMORIA": 8003,
            "NUMERO_MEMORIA": 3,
            "QUANTUM": 3,
            "MULTIPROCESAMIENTO": 1,
            "METADATA_REFRESH": 15000,
            "MEMORIAS_REFRESH": 5000,
            "SLEEP_EJECUCION": 1000
        },
        "Metadata.bin": {
            "BLOCK_SIZE": 128,
            "BLOCKS": 4096,
            "MAGIC_NUMBER": "LISSANDRA"
        }
    }
}

def compile_and_move_executables(project_dir):
    chdir(project_dir + '/Lissandra')
    for target in ['lfs', 'memoria', 'kernel']:
        system('make ' + target)
    system('cp {project_dir}/Lissandra/build/*.out {project_dir}/deployed'
        .format(project_dir = project_dir))
    chdir(project_dir + '/deployed')
    for i in range(1, 6):
        mkdir(project_dir + '/deployed/memoria' + str(i))
        system("cp memoria.out memoria" + str(i))

def create_config_string(process, config):
    res = ''
    for key, value in config.items():
        res += key + '=' + str(value).replace("'", "") + '\n'
    return res

def create_config_for_test(test, config):
    mkdir(test)
    for process, config in config.items():
        cfg_string = create_config_string(process, config)
        f = open(test + '/' + process, 'w')
        f.write(cfg_string)
        f.close()

def create_config_files(project_dir):
    for test, config in configs.items():
        create_config_for_test(test, config)

def deploy():
    file_dir = abspath(dirname(argv[0]))
    original_pwd = getcwd()
    system('rm -r ' + file_dir + '/deployed')
    mkdir(file_dir + '/deployed')
    compile_and_move_executables(file_dir)
    create_config_files(file_dir)
    chdir(original_pwd)

if __name__ == '__main__':
    IP_FS = input("Ingresar ip del lfs: ")
    IP_MEMORIA_1 = input("Ingresar ip de la memoria 1: ")
    IP_MEMORIA_2 = input("Ingresar ip de la memoria 2: ")
    IP_MEMORIA_3 = input("Ingresar ip de la memoria 3: ")
    IP_MEMORIA_4 = input("Ingresar ip de la memoria 4: ")
    IP_MEMORIA_5 = input("Ingresar ip de la memoria 5: ")
    IP_KERNEL = input("Ingresar ip del kernel: ")
    deploy()
