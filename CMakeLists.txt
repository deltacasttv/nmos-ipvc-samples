cmake_minimum_required(VERSION 3.19)

project(
   nmos-ipvc-samples
   DESCRIPTION "NMOS samples for the IPVirtualCard"
   LANGUAGES CXX)

add_subdirectory(nmos-cpp/Development/)
add_subdirectory(video-viewer/)
add_subdirectory ("src")
