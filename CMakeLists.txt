cmake_minimum_required(VERSION 2.8)

project(pin_wss)

option(PIN_ROOT "root path to pin directory" OFF)

if(NOT PIN_ROOT)
   message(FATAL_ERROR "-DPIN_ROOT=path argument missing")
endif()

add_definitions(-Wall -Werror -D__PIN__=1 -DPIN_CRT=1 -O3 -fomit-frame-pointer -fno-strict-aliasing -fno-stack-protector -fno-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX -fabi-version=2)
include_directories(${PIN_ROOT}/source/include/pin)
include_directories(${PIN_ROOT}/source/include/pin/gen)
include_directories(SYSTEM ${PIN_ROOT}/extras/stlport/include)
include_directories(SYSTEM ${PIN_ROOT}/extras/libstdc++/include)
include_directories(SYSTEM ${PIN_ROOT}/extras/crt/include)
include_directories(SYSTEM ${PIN_ROOT}/extras/crt/include/arch-x86_64)
include_directories(SYSTEM ${PIN_ROOT}/extras/crt/include/kernel/uapi)
include_directories(SYSTEM ${PIN_ROOT}/extras/crt/include/kernel/uapi/asm-x86)
include_directories(SYSTEM ${PIN_ROOT}/extras/components/include)
include_directories(SYSTEM ${PIN_ROOT}/extras/xed-intel64/include/xed)

add_library(wss SHARED wss.cpp)

