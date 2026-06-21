
#include <iostream>

#include "include/version.h"


void print_executable_info(){

    std::cout << "............................................." << std::endl;
    std::cout << "PROJECT_VERSION_MAJOR: " << PROJECT_VERSION_MAJOR << std::endl;
    std::cout << "PROJECT_VERSION_MINOR: " << PROJECT_VERSION_MINOR << std::endl;
    std::cout << "PROJECT_VERSION_PATCH: " << PROJECT_VERSION_PATCH << std::endl;
    std::cout << "PROJECT_VERSION_STRING: " << PROJECT_VERSION_STRING << std::endl;
    std::cout << "PROJECT_NAME_STRING: " << PROJECT_NAME_STRING << std::endl;
    std::cout << "BUILD_TYPE: " << BUILD_TYPE << std::endl;
    std::cout << "CMAKE_SYSTEM_NAME: " << CMAKE_SYSTEM_NAME << std::endl;
    std::cout << "BUILD_TIMESTAMP: " << BUILD_TIMESTAMP << std::endl;
    std::cout << "GIT_HASH: " << GIT_HASH << std::endl;
    std::cout << "............................................." << std::endl;
}

int main() {
    print_executable_info();
    return 0;
}
