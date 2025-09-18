cmake_minimum_required(VERSION 3.26)
set (CMAKE_CXX_STANDARD 20)
set( CMAKE_CXX_COMPILER "/usr/bin/clang++" )
set( CMAKE_BUILD_TYPE RelWithDebInfo )
project(FoodServer)

add_executable(server
	main.cpp
	MessageManager.cpp MessageManager.cpp
	TCPCommunicator.cpp TCPCommunicator.hpp
	UDPCommunicator.cpp UDPCommunicator.hpp
   	NetCommunicator.hpp
	dbConnector.cpp dbConnector.hpp
	Server.cpp Server.hpp
	NetMessenger.cpp NetMessenger.hpp
	MessageManager.cpp MessageManager.cpp
	httpreq.cpp httpreq.hpp
)

add_executable(tests
	test_src/test.cpp
	MessageManager.cpp MessageManager.cpp
	TCPCommunicator.cpp TCPCommunicator.hpp
	UDPCommunicator.cpp UDPCommunicator.hpp
	NetCommunicator.hpp
	dbConnector.cpp dbConnector.hpp
	Server.cpp Server.hpp
	NetMessenger.cpp NetMessenger.hpp
	FileManager.cpp FileManager.hpp
	MessageManager.cpp MessageManager.cpp
	httpreq.cpp httpreq.hpp
)


include_directories("/usr/include/mariadb") #path to include folder
add_library(mariadbcpp STATIC IMPORTED)
set_property(TARGET mariadbcpp PROPERTY IMPORTED_LOCATION "/usr/lib/libmariadbcpp.so")

find_package(Catch2 3 REQUIRED)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain mariadbcpp)
target_link_libraries(server mariadbcpp)

