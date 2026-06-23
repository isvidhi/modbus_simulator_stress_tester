#pragma once
#ifndef MODBUS_SIMULATOR
#define MODBUS_SIMULATOR

#include <modbus.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <mutex>
#include <random>
#include <algorithm>
#include <atomic>

#include "include/modbus_config.hpp"
#include "include/modbus_factory.hpp"

const int FUNCTION_CODE_INDEX = 7;

class ModbusSimulator {
private:
    modbus_t* ctx;
    modbus_mapping_t* mapping;
    ModbusConfig::Type type;

    // Validation helper
    bool is_valid(int addr, int count, int max_limit);
    // Write functions for different data types
    bool write_holding_register(int address, uint16_t value);
    bool write_coil(int address, uint8_t value);
    bool write_holding_registers(int address, int count, const uint16_t* values);
    // void process_request(ctx, query, rc);

    std::mutex mtx;
    std::atomic<bool> running{false};
    int listen_socket = -1;
    SimMode current_mode = SimMode::MANUAL;

public:
    ModbusSimulator(const ModbusConfig& config);
    ~ModbusSimulator();
    void run();
    void reset_registers();
    void start_simulation_thread();
    void set_mode(SimMode mode);
};

#endif // MODBUS_SIMULATOR
