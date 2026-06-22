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
                // TCP: Setup Listener
                int server_socket = modbus_tcp_listen(ctx, 1);
                if (server_socket != -1) {
                    modbus_tcp_accept(ctx, &server_socket);
                    connected = true;
                    running = true;
                    start_simulation_thread();
                    std::cout << "TCP Master connected." << std::endl;
                }
                else{
                    running = false;
                }
            } else {
                // RTU: Simple Connect
                if (modbus_connect(ctx) != -1) {
                    connected = true;
                    running = true;
                    std::cout << "RTU Bus connected." << std::endl;
                }
                else{
                    running = false;
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

                // Holding Registers (FC 06, 16)
                // If query[7] == 0x06: The master wrote a single Register.
                // If query[7] == 0x10: The master wrote multiple Registers.
                if (function_code == 0x06 || function_code == 0x10) {
                    // Run the delta-check loop to scan for changes (The "Delta" check)
                    for (int i = 0; i < mapping->nb_registers; ++i) {
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
                    for (int i = 0; i < mapping->nb_bits; ++i) {
                        if (mapping->tab_bits[i] != prev_coils[i]) {
                            std::cout << "[COIL LOG] Coil " << i
                                      << ": " << (int)prev_coils[i] << " -> " << (int)mapping->tab_bits[i] << std::endl;

                            // Update our snapshot
                            prev_coils[i] = mapping->tab_bits[i];
                        }
                    }
                }

            } else {
                // Handle Disconnection
                std::cout << "Connection lost. Resetting..." << std::endl;
                modbus_close(ctx);
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

        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            std::lock_guard<std::mutex> lock(mtx); // Protect access
            for (int i = 0; i < mapping->nb_registers; ++i) {
                int current = mapping->tab_registers[i];
                int new_val = current + delta_dist(gen);
                std::cout << "[REG LOG] Holding Register " << i
                          << ": " << current << " -> " << new_val << std::endl;
                // Clamp between 0 and 1000
                mapping->tab_registers[i] = std::max(0, std::min(1000, new_val));
            }
        }
    }).detach();
}
