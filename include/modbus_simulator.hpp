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
    SimMode current_mode = SimMode::RANDOM;

public:
    ModbusSimulator(const ModbusConfig& config);
    ~ModbusSimulator();
    void run();
    void reset_registers();
    void start_simulation_thread();

    void set_mode(SimMode mode);
    // Strategy for 16-bit Registers
    void apply_strategy_16bit(uint16_t &reg, std::mt19937 &gen);
    // Strategy for Coils/Inputs
    void apply_strategy_bit(uint8_t &bit, std::mt19937 &gen);
};

#endif // MODBUS_SIMULATOR
