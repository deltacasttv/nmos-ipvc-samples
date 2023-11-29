cmake_minimum_required(VERSION 3.19)

if(TARGET video-viewer)
   link_libraries(video-viewer)
endif()

set(receiver_SOURCE
   ${receiver_SOURCE_DIR}receiver.cpp
   ${receiver_SOURCE_DIR}../tools.cpp
   ${receiver_SOURCE_DIR}../nmos_tools.cpp
)

set(receiver_HEADER
   ${receiver_SOURCE_DIR}../tools.h
   ${receiver_SOURCE_DIR}../nmos_tools.h
)

if(UNIX)
    set(receiver_SOURCE
        ${receiver_SOURCE}
        ${receiver_SOURCE_DIR}../keyboard.cpp
    )
    set(receiver_HEADER
        ${receiver_HEADER}
        ${receiver_SOURCE_DIR}../keyboard.h
    )
endif()

add_executable(receiver
               ${receiver_SOURCE}
               ${receiver_HEADER}
)

target_compile_features(receiver PRIVATE cxx_std_17)