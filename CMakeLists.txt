cmake_minimum_required(VERSION 3.28...3.30)
project(obs-gaming-companion VERSION 0.5.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_library(${PROJECT_NAME} MODULE
    src/plugin-main.cpp
    src/gaming-companion-dock.cpp
    src/gaming-companion-dock.hpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OBS::libobs
    OBS::obs-frontend-api
    Qt6::Core
    Qt6::Widgets
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "obs-gaming-companion"
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Psapi)
    # This layout mirrors OBS Studio's Windows plugin layout.
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "obs-plugins/64bit"
        LIBRARY DESTINATION "obs-plugins/64bit"
    )
    install(DIRECTORY data/ DESTINATION "data/obs-plugins/obs-gaming-companion")
else()
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "lib/obs-plugins")
    install(DIRECTORY data/ DESTINATION "share/obs/obs-plugins/obs-gaming-companion")
endif()
