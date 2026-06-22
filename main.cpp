
#include <iostream>
#include "include/version.h"
#include "include/modbus_simulator.hpp"

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

    ModbusConfig config;
    config.type = ModbusConfig::Type::TCP;
    config.ip = "127.0.0.1"; // Listening on localhost
    config.port = 5020;      // Standard port for Modbus simulations

    // Set memory size
    config.map = {1000, 1000, 1000, 1000};

    try {
        // Create simulator: 100 Coils, 100 Input bits, 100 Holding Regs, 100 Input Regs
        // ModbusConfig config{ModbusConfig::Type::RTU, "", 0, "/dev/ttyUSB0", 9600, 'N', 8, 1, 1};
        ModbusSimulator sim(config);
        std::cout << "Starting Modbus TCP Simulator on "
                  << config.ip << ":" << config.port << "..." << std::endl;
        sim.run();
    } catch (const std::exception& e) {
        std::cerr << "Failed to start simulator: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
