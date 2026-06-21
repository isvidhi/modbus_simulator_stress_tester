#include <include/modbus_simulator.hpp>

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
    if (mapping){
        modbus_mapping_free(mapping);
    }
    // ... cleanup ctx
    modbus_close(ctx);
    modbus_free(ctx);
}

void ModbusSimulator::run() {
    std::cout << "[ModbuSimulator] run ( " << this << " )" << std::endl;
    if (type == ModbusConfig::Type::TCP) {
        int s = modbus_tcp_listen(ctx, 1);
        modbus_tcp_accept(ctx, &s);
    }
    else{
        modbus_connect(ctx); // Necessary for RTU
    }

    // Shared loop logic
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
    while(true){

        int rc = modbus_receive(ctx, query);
        if(rc > 0){
            modbus_reply(ctx, query, rc, mapping);
        }
    }
}
