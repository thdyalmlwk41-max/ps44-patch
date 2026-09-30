cmake_minimum_required(VERSION 3.16)
project(days_gone_900p LANGUAGES C ASM)

# OrbisDev toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET x86_64-scei-ps4)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_ASM_COMPILER_TARGET x86_64-scei-ps4)
set(CMAKE_LINKER lld)
set(CMAKE_AR llvm-ar)
set(CMAKE_RANLIB llvm-ranlib)
set(CMAKE_OBJCOPY llvm-objcopy)

# Compiler flags for PS4 bare metal
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -fPIC -nostdlib -fno-builtin -fno-stack-protector -mno-sse -mno-avx -mcmodel=large -ffreestanding -fno-exceptions -fno-rtti")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -nostdlib")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined -Wl,--gc-sections -Wl,-z,max-page-size=0x10000 -nostdlib -Wl,-T,linker.x")
set(CMAKE_SHARED_MODULE_FLAGS "${CMAKE_SHARED_MODULE_FLAGS} -Wl,--no-undefined -Wl,--gc-sections")

# Output as .sprx (PS4 shared library)
set(CMAKE_MODULE_SUFFIX ".sprx")

add_library(days_gone_900p MODULE
    src/main.c
    src/vm.c
    src/pattern.c
    src/hook.c
    src/ue4.c
)

target_include_directories(days_gone_900p PRIVATE include)
target_compile_definitions(days_gone_900p PRIVATE 
    ORBIS 
    __ORBIS__ 
    _ORBIS_SDK_VERSION_=9000000
)

# Custom linker script for SPRX
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/linker.x.in ${CMAKE_CURRENT_BINARY_DIR}/linker.x @ONLY)

install(TARGETS days_gone_900p DESTINATION .)