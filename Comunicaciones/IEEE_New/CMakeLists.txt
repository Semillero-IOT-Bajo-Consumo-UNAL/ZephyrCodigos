# Indicamos la version minima de CMAKE
cmake_minimum_required(VERSION 3.20.0)

# Imporamos Zephyr como obligatorio
find_package(Zephyr REQUIRED)

# Le ponemos nombre a nuestro proyecto ( puramente para informacion )
project(IEEE)

# si vamos a incluir mas librerias descomentamos e indicamos la carpeta de los .h
# target_include_directories(app PRIVATE libreria/carpeta/include/)

# Indicamos los archivos de NUESTRO proyecto vamos a compilar
FILE (GLOB app_sources src/*.c)
target_sources(app PRIVATE src/main.c)