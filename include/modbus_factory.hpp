#pragma once
#ifndef MODBUS_FACTORY
#define MODBUS_FACTORY

#include <iostream>
#include "include/modbus_config.hpp"

class ModbusFactory {
public:
    static modbus_t* create_context(const ModbusConfig& config);
};

#endif // MODBUS_FACTORY
