#!/usr/bin/python3

from os.path import abspath, dirname
from os import chdir, system, getcwd, mkdir
from sys import argv

config_memoria = """PUERTO_ESCUCHA=8001
IP_FS=127.0.0.1
PUERTO_FS=5003
IP_SEEDS=[]
PUERTO_SEEDS=[]
RETARDO_MEM=600
RETARDO_FD=600
TAM_MEM=4096
RETARDO_JOURNAL=70000
RETARDO_GOSSIPING=30000
MEMORY_NUMBER=1
"""

config_kernel = """IP_MEMORIA=127.0.0.1
PUERTO_MEMORIA=8001
NUMERO_MEMORIA=1
QUANTUM=3
MULTIPROCESAMIENTO=1
METADATA_REFRESH=15000
MEMORIAS_REFRESH=15000
SLEEP_EJECUCION=100
"""

config_lfs = """PUERTO_ESCUCHA=5003
PUNTO_MONTAJE=/home/utnso/lissandra-checkpoint/
RETARDO=100
TAMANIO_VALUE=255
TIEMPO_DUMP=60000
"""

def compile_and_move_executables(project_dir):
    for target in ['lfs', 'memoria', 'kernel']:
        system('make ' + target)
    system('cp {project_dir}/Lissandra/build/*.out {project_dir}/deployed'
        .format(project_dir = project_dir))
    system('make clean')

def create_config_files(project_dir):
    cfg_files = ['lfs.conf', 'memoria.conf', 'kernel.conf']
    cfg_files = list(map(lambda cfg_file: project_dir + '/deployed/' + cfg_file, cfg_files))
    cfg_strings = [config_lfs, config_memoria, config_kernel]
    for cfg_file, cfg_string in list(zip(cfg_files, cfg_strings)):
        f = open(cfg_file, 'w')
        f.write(cfg_string)
        f.close()

def deploy():
    file_dir = abspath(dirname(argv[0]))
    original_pwd = getcwd()
    system('rm -r ' + file_dir + '/deployed')
    mkdir(file_dir + '/deployed')
    chdir(file_dir + '/Lissandra')
    compile_and_move_executables(file_dir)
    create_config_files(file_dir)
    chdir(original_pwd)

if __name__ == '__main__':
    deploy()
