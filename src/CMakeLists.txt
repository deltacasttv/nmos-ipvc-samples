cmake_minimum_required(VERSION 3.19)

if(WIN32)
   include_directories($ENV{VIDEOMASTERIP})
   link_directories($ENV{VIDEOMASTERIP_LIB}/x64)
   link_libraries(videomasterip2)
endif()
if(UNIX)
   link_libraries(videomasterip) #pthread
endif()

if(TARGET nmos-cpp::nmos-cpp)
   link_libraries(nmos-cpp::nmos-cpp)
endif()

add_subdirectory(sender)
add_subdirectory(receiver)
