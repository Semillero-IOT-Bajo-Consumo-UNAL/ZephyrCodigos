cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(as7341_raw_reading)


target_include_directories(app PRIVATE include/)

file(GLOB zsl "lib/zsl/*.c")
file(GLOB colorimetryLib "lib/zsl/colorimetry/*.c")
file(GLOB orientationLib "lib/zsl/orientation/*.c")
file(GLOB physicsLib "lib/zsl/physics/*.c")
# Add source files
target_sources(app PRIVATE 
    src/main.c
    src/as7341.c   
    ${zsl}
    ${colorimetryLib}
    ${orientationLib}
    ${physicsLib}
)






