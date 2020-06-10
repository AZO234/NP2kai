# OpenDingux
set(OPENDINGUX TRUE)

# cross compiler settings
set(CMAKE_CROSSCOMPILING TRUE)
set(CROSS_COMPILE "/opt/rg350-toolchain/usr/bin/mipsel-linux-")
set(CMAKE_C_COMPILER ${CROSS_COMPILE}gcc)
set(CMAKE_CXX_COMPILER ${CROSS_COMPILE}g++)
set(CMAKE_LINKER ${CROSS_COMPILE}gcc)

# root path settings
set(CMAKE_FIND_ROOT_PATH "/opt/rg350-toolchain/usr")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)  # use host system root for program
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)   # use CMAKE_FIND_ROOT_PATH for library
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)   # use CMAKE_FIND_ROOT_PATH for include

set(ENV{RPATH} $ENV{RPATH} "/opt/rg350-toolchain/usr/lib")

