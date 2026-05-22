# 1. On définit une version standard existante
cmake_minimum_required(VERSION 3.10)
project(socialMedia)

# On utilise le standard C++14 (très bien !)
set(CMAKE_CXX_STANDARD 14)

# 2. On déclare D'ABORD la création du programme (notre cible s'appelle "socialMedia")
# Il sera compilé à partir de ton fichier "server.cpp"
add_executable(socialMedia server.cpp)

# 3. On cherche la librairie SQLite3 sur ton Mac
find_package(SQLite3 REQUIRED)

# 4. On lie SQLite à notre cible (remarque que l'on utilise bien "socialMedia" ici aussi)
target_link_libraries(socialMedia SQLite::SQLite3)