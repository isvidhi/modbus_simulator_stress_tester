#pragma once
#ifndef MODBUS_SIMULATOR
#define MODBUS_SIMULATOR

#include <modbus.h>
#include <iostream>
#include <vector>
#include <stdexcept>

#include "include/modbus_config.hpp"
#include "include/modbus_factory.hpp"

class ModbusSimulator {
private:
    modbus_t* ctx;
    modbus_mapping_t* mapping;
    ModbusConfig::Type type;

public:
    ModbusSimulator(const ModbusConfig& config);
    ~ModbusSimulator();
    void run();
};

#endif // MODBUS_SIMULATOR
