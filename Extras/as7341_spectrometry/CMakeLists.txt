cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(as7341_spectrometry)


file(GLOB zsl "lib/zsl/*.c")
file(GLOB colorimetryLib "lib/zsl/colorimetry/*.c")
file(GLOB orientationLib "lib/zsl/orientation/*.c")
file(GLOB physicsLib "lib/zsl/physics/*.c")

target_include_directories(app PRIVATE include/)
target_sources(app PRIVATE 
src/main.c
src/as7341.c
${zsl}
${colorimetryLib}
${orientationLib}
${physicsLib}
)










