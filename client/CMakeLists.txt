# CMakeList.txt : CMake project for client, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (
	client 
	"client.cpp"
	"client.h"
)

target_link_libraries(client ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET client PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
