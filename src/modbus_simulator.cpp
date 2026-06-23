#include <include/modbus_simulator.hpp>

bool ModbusSimulator::is_valid(int addr, int count, int max_limit) {
    return (addr >= 0 && (addr + count) <= max_limit);
}

bool ModbusSimulator::write_holding_register(int address, uint16_t value) {
    if (!is_valid(address, 1, mapping->nb_registers)){
        return false;
    }
    mapping->tab_registers[address] = value;
    return true;
}

bool ModbusSimulator::write_coil(int address, uint8_t value) {
    if (!is_valid(address, 1, mapping->nb_bits)){
        return false;
    }
    mapping->tab_bits[address] = value;
    return true;
}

bool ModbusSimulator::write_holding_registers(int address, int count, const uint16_t *values) {
    if (!is_valid(address, count, mapping->nb_registers)){
        return false;
    }
    for (int i = 0; i < count; ++i) {
        mapping->tab_registers[address + i] = values[i];
    }
    return true;
}

ModbusSimulator::ModbusSimulator(const ModbusConfig &config) : type(config.type){
    std::cout << "[ModbuSimulator] ctor ( " << this << " )" << std::endl;
    ctx = ModbusFactory::create_context(config);

    // Initialize mapping using the user-defined sizes
    mapping = modbus_mapping_new(
        config.map.nb_coils,
        config.map.nb_input_bits,
        config.map.nb_holding_registers,
        config.map.nb_input_registers
    );

    if (!mapping) {
        // Handle error: e.g., memory allocation failed
        throw std::runtime_error(modbus_strerror(errno));
    }
}

ModbusSimulator::~ModbusSimulator() {
    std::cout << "[ModbuSimulator] dtor ( " << this << " )" << std::endl;
    running = false; // Signal the background thread to stop
    // Give the thread a moment to finish its current loop before destroying memory
    // (Optional: join the thread if you stored the std::thread object)
    if (mapping){
        modbus_mapping_free(mapping);
    }
    // ... cleanup ctx
    if(ctx){
        modbus_close(ctx);
        modbus_free(ctx);
    }
}

// void ModbusSimulator::run() {
//     std::cout << "[ModbuSimulator] run ( " << this << " )" << std::endl;
//     if (type == ModbusConfig::Type::TCP) {
//         int s = modbus_tcp_listen(ctx, 1);
//         modbus_tcp_accept(ctx, &s);
//     }
//     else{
//         modbus_connect(ctx); // Necessary for RTU
//     }

//     // Shared loop logic
//     uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

//     // To monitor Holding Registers and Coils
//     // Create a buffer to hold the previous state of Holding Registers
//     std::vector<uint16_t> prev_registers(mapping->nb_registers);

//     // Create a buffer to hold the previous state of Coils
//     std::vector<uint8_t> prev_coils(mapping->nb_bits);

//     // Initialize Holding Register Buffer
//     for(int i = 0; i < mapping->nb_registers; ++i){
//         prev_registers[i] = mapping->tab_registers[i];
//     }

//     // Initialize Holding Register Buffer
//     for(int i=0; i < mapping->nb_bits; ++i){
//         prev_coils[i] = mapping->tab_bits[i];
//     }

//     while(true){

//         int rc = modbus_receive(ctx, query);

//         if (rc > 0) {
//             // modbus_reply handles everything:
//             // 1. Decodes function codes (Read Coils, Write Regs, etc.)
//             // 2. Validates address ranges against mb_mapping
//             // 3. Modifies mb_mapping (on Write) or reads from it (on Read)
//             // 4. Sends the response back to the Master
//             modbus_reply(ctx, query, rc, mapping);

//             // introspection for write
//             uint8_t function_code = query[FUNCTION_CODE_INDEX];

//             // Holding Registers (FC 06, 16)
//             // If query[7] == 0x06: The master wrote a single Register.
//             // If query[7] == 0x10: The master wrote multiple Registers.
//             if (function_code == 0x06 || function_code == 0x10) {
//                 // Run the delta-check loop to scan for changes (The "Delta" check)
//                 for (int i = 0; i < mapping->nb_registers; ++i) {
//                     if (mapping->tab_registers[i] != prev_registers[i]) {
//                         std::cout << "[REG LOG] Holding Register " << i
//                                   << " changed: " << prev_registers[i]
//                                   << " -> " << mapping->tab_registers[i]
//                                   << std::endl;

//                         // Update our snapshot
//                         prev_registers[i] = mapping->tab_registers[i];
//                     }
//                 }
//             }

//             // Coils (FC 05, 15)
//             // If query[7] == 0x05: The master wrote a single Coil.
//             // If query[7] == 0x0f: The master wrote multiple Coils.
//             if (function_code == 0x05 || function_code == 0x0f) {
//                 for (int i = 0; i < mapping->nb_bits; ++i) {
//                     if (mapping->tab_bits[i] != prev_coils[i]) {
//                         std::cout << "[COIL LOG] Coil " << i
//                                   << ": " << (int)prev_coils[i] << " -> " << (int)mapping->tab_bits[i] << std::endl;

//                         // Update our snapshot
//                         prev_coils[i] = mapping->tab_bits[i];
//                     }
//                 }
//             }

//         } else if (rc == -1) {
//             // Connection closed or error
//             break;
//         }
//     }
// }

void ModbusSimulator::run() {
    bool connected = false;
    listen_socket = -1;

    // Setup Listener ONCE, outside the loop
    if (type == ModbusConfig::Type::TCP) {
        listen_socket = modbus_tcp_listen(ctx, 1);
        if (listen_socket == -1) {
            std::cerr << "Fatal: Could not listen: " << modbus_strerror(errno) << std::endl;
            return;
        }
    }

    // To monitor Holding Registers and Coils
    // Create a buffer to hold the previous state of Holding Registers
    std::vector<uint16_t> prev_registers(mapping->nb_registers);

    // Create a buffer to hold the previous state of Coils
    std::vector<uint8_t> prev_coils(mapping->nb_bits);

    // Initialize Holding Register Buffer
    for(int i = 0; i < mapping->nb_registers; ++i){
        prev_registers[i] = mapping->tab_registers[i];
    }

    // Initialize Holding Register Buffer
    for(int i=0; i < mapping->nb_bits; ++i){
        prev_coils[i] = mapping->tab_bits[i];
    }

    while (true) {
        if (!connected) {
            if (type == ModbusConfig::Type::TCP) {
                // Accept using the existing listen_socket
                int server_socket = modbus_tcp_accept(ctx, &listen_socket);
                if (server_socket != -1) {
                    // modbus_tcp_accept(ctx, &server_socket);
                    connected = true;
                    running = true;
                    start_simulation_thread();
                    std::cout << "TCP Master connected: " << std::endl;
                }
                else{
                    running = false;
                    std::cout << "TCP Master Not connected: " << modbus_strerror(errno) << std::endl;
                    if(ctx){
                        modbus_close(ctx);
                    }
                    // If TCP, ensure we reset the internal context state if needed
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
            else {
                // RTU: Simple Connect
                if (modbus_connect(ctx) != -1) {
                    connected = true;
                    running = true;
                    std::cout << "RTU Bus connected: " << std::endl;
                }
                else{
                    running = false;
                    std::cout << "RTU Master Not connected: " << modbus_strerror(errno) << std::endl;
                    if(ctx){
                        modbus_close(ctx);
                    }
                    // If TCP, ensure we reset the internal context state if needed
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
        }

        if (connected) {
            uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
            int rc = modbus_receive(ctx, query);

            if (rc > 0) {
                // process_request(ctx, query, rc, mapping); // Your logic with logging

                // LOCK during network processing
                std::lock_guard<std::mutex> lock(mtx);

                // modbus_reply handles everything:
                // 1. Decodes function codes (Read Coils, Write Regs, etc.)
                // 2. Validates address ranges against mb_mapping
                // 3. Modifies mb_mapping (on Write) or reads from it (on Read)
                // 4. Sends the response back to the Master
                modbus_reply(ctx, query, rc, mapping);

                // introspection for write
                uint8_t function_code = query[FUNCTION_CODE_INDEX];
                // Extract address and quantity from the query buffer
                // The address starts at byte 8, quantity at byte 10
                // The Address Calculation: (query[8] << 8) | query[9]
                // query[8] << 8: This takes the byte at index 8 and shifts it 8 bits to the left. This moves the value into the "high" position (making it the upper 8 bits of a 16-bit number).
                // | (Bitwise OR): This combines the shifted high byte with the byte at index 9 (the "low" byte).
                //     Example: If the address is 200 (which is 0x00C8 in hex):
                //     query[8] is 0x00
                //     query[9] is 0xC8
                //     0x00 << 8 becomes 0x0000 and 0x0000 | 0xC8 = 0x00C8 (Decimal 200).
                // same logic to extract quantity from 10 and 11 index.
                int address = (query[8] << 8) | query[9];
                int quantity = (query[10] << 8) | query[11];

                // Holding Registers (FC 06, 16)
                // If query[7] == 0x06: The master wrote a single Register.
                // If query[7] == 0x10: The master wrote multiple Registers.
                if (function_code == 0x06 || function_code == 0x10) {
                    // Run the delta-check loop to scan for changes (The "Delta" check)
                    for (int i = address; i < quantity; ++i) {
                        if (mapping->tab_registers[i] != prev_registers[i]) {
                            std::cout << "[REG LOG] Holding Register " << i
                                      << " changed: " << prev_registers[i]
                                      << " -> " << mapping->tab_registers[i]
                                      << std::endl;

                            // Update our snapshot
                            prev_registers[i] = mapping->tab_registers[i];
                        }
                    }
                }

                // Coils (FC 05, 15)
                // If query[7] == 0x05: The master wrote a single Coil.
                // If query[7] == 0x0f: The master wrote multiple Coils.
                if (function_code == 0x05 || function_code == 0x0f) {
                    for (int i = address; i < quantity; ++i) {
                        if (mapping->tab_bits[i] != prev_coils[i]) {
                            std::cout << "[COIL LOG] Coil " << i
                                      << ": " << (int)prev_coils[i] << " -> " << (int)mapping->tab_bits[i] << std::endl;

                            // Update our snapshot
                            prev_coils[i] = mapping->tab_bits[i];
                        }
                    }
                }

                // Read Holding Registers (FC 03) and Input Registers (FC 04)
                if (function_code == 0x03 || function_code == 0x04) {
                    std::cout << "[READ LOG] " << (function_code == 0x03 ? "Holding Register" : "Input Register")
                              << " access: Addr=" << address
                              << ", Count=" << quantity << std::endl;
                }

                // Read Coils (FC 01) and Discrete Inputs (FC 02)
                else if (function_code == 0x01 || function_code == 0x02) {
                    std::cout << "[READ LOG] " << (function_code == 0x01 ? "Coil" : "Discrete Input")
                              << " access: Addr=" << address
                              << ", Count=" << quantity << std::endl;
                }

            } else {
                // Handle Disconnection
                std::cout << "Connection lost. Resetting..." << std::endl;

                // Cleanup current session
                running = false; // Stop simulation thread
                modbus_close(ctx); // Close the client connection
                connected = false;

                // Optional: Short sleep to prevent CPU hammering during error
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
}

void ModbusSimulator::reset_registers() {
    // Zero out holding registers
    std::fill(mapping->tab_registers, mapping->tab_registers + mapping->nb_registers, 0);
    // Zero out coils
    std::fill(mapping->tab_bits, mapping->tab_bits + mapping->nb_bits, 0);
}

void ModbusSimulator::ModbusSimulator::start_simulation_thread() {
    std::cout << "[ModbusSimulator] start_simulation_thread" << std::endl;
    std::thread([this]() {
        std::cout << "[ModbusSimulator] start_simulation_thread Lambda" << std::endl;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> delta_dist(-5, 5); // Smooth change

        // while (running) {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(500));

        //     std::lock_guard<std::mutex> lock(mtx); // Protect access
        //     for (int i = 0; i < mapping->nb_registers; ++i) {
        //         int current = mapping->tab_registers[i];
        //         int new_val = current + delta_dist(gen);
        //         // std::cout << "[REG LOG] Holding Register " << i
        //         //           << ": " << current << " -> " << new_val << std::endl;
        //         // Clamp between 0 and 1000
        //         mapping->tab_registers[i] = std::max(0, std::min(1000, new_val));
        //     }
        // }
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::lock_guard<std::mutex> lock(mtx);

            int current = 0;
            int new_val = 0;

            // 1. Holding Registers & Input Registers (16-bit)
            for (int i = 0; i < mapping->nb_registers; ++i){
                current = mapping->tab_registers[i];
                apply_strategy_16bit(mapping->tab_registers[i], gen);
                new_val = mapping->tab_registers[i];
                std::cout << "[REG LOG] Holding Register addr:" << i << ": " << current << " -> " << new_val << std::endl;
            }
            for (int i = 0; i < mapping->nb_input_registers; ++i){
                current = mapping->tab_input_registers[i];
                apply_strategy_16bit(mapping->tab_input_registers[i], gen);
                new_val = mapping->tab_input_registers[i];
                std::cout << "[I-REG LOG] Input Register addr:" << i << ": " << current << " -> " << new_val << std::endl;
            }

            // 2. Coils & Discrete Inputs (Boolean/Bit)
            for (int i = 0; i < mapping->nb_bits; ++i){
                current = mapping->tab_bits[i];
                apply_strategy_bit(mapping->tab_bits[i], gen);
                new_val = mapping->tab_bits[i];
                std::cout << "[COIL LOG] Coil addr:" << i << ": " << current << " -> " << new_val << std::endl;
            }
            for (int i = 0; i < mapping->nb_input_bits; ++i){
                current = mapping->tab_input_bits[i];
                apply_strategy_bit(mapping->tab_input_bits[i], gen);
                new_val = mapping->tab_input_bits[i];
                std::cout << "[I-COIL LOG] Input Coil addr:" << i << ": " << current << " -> " << new_val << std::endl;
            }
        }
    }).detach();
}

void ModbusSimulator::set_mode(SimMode mode) {
    std::lock_guard<std::mutex> lock(mtx);
    current_mode = mode;
}

void ModbusSimulator::apply_strategy_16bit(uint16_t &reg, std::mt19937 &gen) {
    if (current_mode == SimMode::RANDOM) {
        std::uniform_int_distribution<int> dist(-5, 5);
        reg = std::max(0, std::min(1000, (int)reg + dist(gen)));
    }
    else if (current_mode == SimMode::INCREMENTAL) {
        reg = (reg + 1) % 1001;
    }
}

void ModbusSimulator::apply_strategy_bit(uint8_t &bit, std::mt19937 &gen) {
    if (current_mode == SimMode::RANDOM) {
        std::uniform_int_distribution<int> dist(0, 1);
        bit = dist(gen);
    }
    // Manual/Incremental usually don't apply to raw bits,
    // but you could add a toggle strategy if desired.
}
