cmake_minimum_required(VERSION 3.19)

set(sender_SOURCE
   ${sender_SOURCE_DIR}sender.cpp
   ${sender_SOURCE_DIR}../tools.cpp
   ${sender_SOURCE_DIR}../nmos_tools.cpp
   ${sender_SOURCE_DIR}pattern.cpp
)

set(sender_HEADER
   ${sender_SOURCE_DIR}../tools.h
   ${sender_SOURCE_DIR}../nmos_tools.h
   ${sender_SOURCE_DIR}pattern.h
)

if(UNIX)
    set(sender_SOURCE
        ${sender_SOURCE}
        ${sender_SOURCE_DIR}../keyboard.cpp
    )
    set(sender_HEADER
        ${sender_HEADER}
        ${sender_SOURCE_DIR}../keyboard.h
    )
endif()

add_executable(sender
               ${sender_SOURCE}
               ${sender_HEADER}
)

target_compile_features(sender PRIVATE cxx_std_17)