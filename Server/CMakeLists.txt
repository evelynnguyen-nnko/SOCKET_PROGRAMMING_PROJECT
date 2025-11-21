# # 0. KHAI BÁO TOOLCHAIN TRƯỚC project()
# set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
#     CACHE STRING "Vcpkg toolchain file")

# cmake_minimum_required(VERSION 3.20)
project(Server)

# show where to find .cpp files
file(GLOB_RECURSE SOURCES 
    "Module_1/*.cpp" 
    "Module_2/*.cpp" 
    "main.cpp"
)

# create execution file .exe
add_executable(${PROJECT_NAME} ${SOURCES})

# show where to find headers
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/Module_1"
    "${CMAKE_CURRENT_SOURCE_DIR}/Module_2"
)

# Link to libraries: Boost + Threads
find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Boost::system
    Threads::Threads
)


# --------------------
#  OPENCV
# --------------------
find_package(OpenCV REQUIRED)

# Include directories của OpenCV
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})

# Link OpenCV
target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenCV_LIBS})