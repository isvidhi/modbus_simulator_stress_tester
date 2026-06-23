#pragma once
#ifndef MODBUS_CONFIG
#define MODBUS_CONFIG

#include <modbus.h>
#include <string>

enum class SimMode { RANDOM, INCREMENTAL, MANUAL };

struct ModbusMemoryMap {
    int nb_coils = 100;
    int nb_input_bits = 100;
    int nb_holding_registers = 100;
    int nb_input_registers = 100;
};

struct ModbusConfig {
    enum class Type { TCP, RTU };
    Type type;

    // TCP Params
    std::string ip;
    int port;

    // RTU Params
    std::string device;
    int baud;
    char parity;
    int data_bit;
    int stop_bit;

    int slave_id;

    // Memory Map Params
    ModbusMemoryMap map;
};

#endif // MODBUS_CONFIG
