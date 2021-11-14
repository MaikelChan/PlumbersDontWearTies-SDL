# CMakeList.txt : CMake project for PlumbersDontWearTies, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (${PROJECT_NAME}
    "Game.cpp"
    "Game.h"
    "GameData.h"
    "main.cpp"
    "main.h"
)

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBS})

# TODO: Add tests and install targets if needed.