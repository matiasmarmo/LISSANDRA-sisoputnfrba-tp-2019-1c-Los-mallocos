#!/usr/bin/python3

import argparse
from os.path import abspath, dirname, exists
from os import chdir, system, getcwd, mkdir
from sys import argv

IP_FS = input("Ingresar ip del lfs: ")
IP_MEMORIA_1 = input("Ingresar ip de la memoria 1: ")
IP_MEMORIA_2 = input("Ingresar ip de la memoria 2: ")
IP_MEMORIA_3 = input("Ingresar ip de la memoria 3: ")
IP_MEMORIA_4 = input("Ingresar ip de la memoria 4: ")
IP_MEMORIA_5 = input("Ingresar ip de la memoria 5: ")
IP_KERNEL = input("Ingresar ip del kernel: ")

configs = {
    "prueba_base": {
        "lfs.conf": {
            "PUERTO_ESCUCHA": 5003,
            "PUNTO_MONTAJE": "/home/ppaglilla/Projects/tp-2019-1c-Los-mallocos/Lissandra",
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
            "PUNTO_MONTAJE": "/home/ppaglilla/Projects/tp-2019-1c-Los-mallocos/Lissandra",
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
            "QUANTUM": 1,
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
    },
    "prueba_lfs": {
        "lfs.conf": {
            "PUERTO_ESCUCHA": 5003,
            "PUNTO_MONTAJE": "/home/ppaglilla/Projects/tp-2019-1c-Los-mallocos/Lissandra",
            "RETARDO": 0,
            "TAMANIO_VALUE": 60,
            "TIEMPO_DUMP": 5000
        },
        "memoria1.conf": {
            "PUERTO_ESCUCHA": 8001,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [IP_MEMORIA_2],
            "PUERTO_SEEDS": [8002],
            "RETARDO_MEM": 0,
            "RETARDO_FS": 0,
            "TAM_MEM": 320,
            "RETARDO_JOURNAL": 60000,
            "RETARDO_GOSSIPING": 10000,
            "MEMORY_NUMBER": 1
        },
        "memoria2.conf": {
            "PUERTO_ESCUCHA": 8002,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [IP_MEMORIA_3],
            "PUERTO_SEEDS": [8003],
            "RETARDO_MEM": 0,
            "RETARDO_FS": 0,
            "TAM_MEM": 320,
            "RETARDO_JOURNAL": 60000,
            "RETARDO_GOSSIPING": 10000,
            "MEMORY_NUMBER": 2
        },
        "memoria3.conf": {
            "PUERTO_ESCUCHA": 8003,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [],
            "PUERTO_SEEDS": [],
            "RETARDO_MEM": 0,
            "RETARDO_FS": 0,
            "TAM_MEM": 320,
            "RETARDO_JOURNAL": 60000,
            "RETARDO_GOSSIPING": 10000,
            "MEMORY_NUMBER": 3
        },
        "kernel.conf": {
            "IP_MEMORIA": IP_MEMORIA_1,
            "PUERTO_MEMORIA": 8001,
            "NUMERO_MEMORIA": 1,
            "QUANTUM": 3,
            "MULTIPROCESAMIENTO": 1,
            "METADATA_REFRESH": 15000,
            "MEMORIAS_REFRESH": 5000,
            "SLEEP_EJECUCION": 10
        },
        "Metadata.bin": {
            "BLOCK_SIZE": 64,
            "BLOCKS": 4096,
            "MAGIC_NUMBER": "LISSANDRA"
        }
    },
    "prueba_memoria": {
        "lfs.conf": {
            "PUERTO_ESCUCHA": 5003,
            "PUNTO_MONTAJE": "/home/ppaglilla/Projects/tp-2019-1c-Los-mallocos/Lissandra",
            "RETARDO": 0,
            "TAMANIO_VALUE": 60,
            "TIEMPO_DUMP": 3000
        },
        "memoria1.conf": {
            "PUERTO_ESCUCHA": 8001,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [],
            "PUERTO_SEEDS": [],
            "RETARDO_MEM": 0,
            "RETARDO_FS": 0,
            "TAM_MEM": 340,
            "RETARDO_JOURNAL": 1000000,
            "RETARDO_GOSSIPING": 10000,
            "MEMORY_NUMBER": 1
        },
        "kernel.conf": {
            "IP_MEMORIA": IP_MEMORIA_1,
            "PUERTO_MEMORIA": 8001,
            "NUMERO_MEMORIA": 1,
            "QUANTUM": 3,
            "MULTIPROCESAMIENTO": 1,
            "METADATA_REFRESH": 15000,
            "MEMORIAS_REFRESH": 5000,
            "SLEEP_EJECUCION": 0
        },
        "Metadata.bin": {
            "BLOCK_SIZE": 64,
            "BLOCKS": 4096,
            "MAGIC_NUMBER": "LISSANDRA"
        }
    },
    "prueba_stress": {
        "lfs.conf": {
            "PUERTO_ESCUCHA": 5003,
            "PUNTO_MONTAJE": "/home/ppaglilla/Projects/tp-2019-1c-Los-mallocos/Lissandra",
            "RETARDO": 0,
            "TAMANIO_VALUE": 60,
            "TIEMPO_DUMP": 20000
        },
        "memoria1.conf": {
            "PUERTO_ESCUCHA": 8001,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [],
            "PUERTO_SEEDS": [],
            "RETARDO_MEM": 0,
            "RETARDO_FS": 0,
            "TAM_MEM": 4096,
            "RETARDO_JOURNAL": 70000,
            "RETARDO_GOSSIPING": 30000,
            "MEMORY_NUMBER": 1
        },
        "memoria2.conf": {
            "PUERTO_ESCUCHA": 8002,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [IP_MEMORIA_1],
            "PUERTO_SEEDS": [8001],
            "RETARDO_MEM": 0,
            "RETARDO_FS": 0,
            "TAM_MEM": 2048,
            "RETARDO_JOURNAL": 30000,
            "RETARDO_GOSSIPING": 30000,
            "MEMORY_NUMBER": 2
        },
        "memoria3.conf": {
            "PUERTO_ESCUCHA": 8003,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [IP_MEMORIA_4, IP_MEMORIA_2],
            "PUERTO_SEEDS": [8004, 8002],
            "RETARDO_MEM": 0,
            "RETARDO_FS": 0,
            "TAM_MEM": 256,
            "RETARDO_JOURNAL": 70000,
            "RETARDO_GOSSIPING": 30000,
            "MEMORY_NUMBER": 3
        },
        "memoria4.conf": {
            "PUERTO_ESCUCHA": 8004,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [IP_MEMORIA_1],
            "PUERTO_SEEDS": [8001],
            "RETARDO_MEM": 0,
            "RETARDO_FS": 0,
            "TAM_MEM": 4096,
            "RETARDO_JOURNAL": 30000,
            "RETARDO_GOSSIPING": 30000,
            "MEMORY_NUMBER": 4
        },
        "memoria5.conf": {
            "PUERTO_ESCUCHA": 8005,
            "IP_FS": IP_FS,
            "PUERTO_FS": 5003,
            "IP_SEEDS": [IP_MEMORIA_2,IP_MEMORIA_3],
            "PUERTO_SEEDS": [8002,8003],
            "RETARDO_MEM": 0,
            "RETARDO_FS": 0,
            "TAM_MEM": 1024,
            "RETARDO_JOURNAL": 50000,
            "RETARDO_GOSSIPING": 30000,
            "MEMORY_NUMBER": 5
        },
        "kernel.conf": {
            "IP_MEMORIA": IP_MEMORIA_5,
            "PUERTO_MEMORIA": 8005,
            "NUMERO_MEMORIA": 5,
            "QUANTUM": 1,
            "MULTIPROCESAMIENTO": 3,
            "METADATA_REFRESH": 15000,
            "MEMORIAS_REFRESH": 5000,
            "SLEEP_EJECUCION": 10
        },
        "Metadata.bin": {
            "BLOCK_SIZE": 128,
            "BLOCKS": 8192,
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
        res += key + '=' + str(value).replace("'", "").replace(" ", "") + '\n'
    return res

def create_config_for_test(test, config):
    #mkdir(test)
    punto_montaje = config['lfs.conf']['PUNTO_MONTAJE']
    punto_montaje = punto_montaje if punto_montaje.endswith('/') else punto_montaje + '/'
    for process, config in config.items():
        cfg_string = create_config_string(process, config)
        file_path = ''
        if process.startswith('lfs') or process.startswith('kernel'):
            file_path = process
        elif process.startswith('memoria'):
            file_path = process.split('.')[0] + '/memoria.conf'
        else:
            # Es el Metadata.bin
            if not exists(punto_montaje):
                mkdir(punto_montaje)
            if not exists(punto_montaje + 'Metadata/'):
                mkdir(punto_montaje + 'Metadata/')
            file_path = punto_montaje + 'Metadata/Metadata.bin'
        f = open(file_path, 'w')
        f.write(cfg_string)
        f.close()

def create_config_files(project_dir, test):
    create_config_for_test(test, configs[test])

def parse_cli_arguments():
    parser = argparse.ArgumentParser(description = "Lissandra deployer")
    parser.add_argument('prueba', choices = {"base", "kernel", "memoria", "lfs", "stress"})
    return parser.parse_args()

def deploy(test):
    file_dir = abspath(dirname(argv[0]))
    original_pwd = getcwd()
    system('rm -r ' + file_dir + '/deployed')
    mkdir(file_dir + '/deployed')
    compile_and_move_executables(file_dir)
    create_config_files(file_dir, test)
    chdir(original_pwd)

if __name__ == '__main__':
    args = parse_cli_arguments()
    args.prueba = 'prueba_' + args.prueba
    deploy(args.prueba)
