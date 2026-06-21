#include "include/modbus_factory.hpp"

modbus_t *ModbusFactory::create_context(const ModbusConfig &config) {
    modbus_t* ctx = nullptr;

    std::cout << "[ModbuFactory] create_context " << std::endl;
    if(config.type == ModbusConfig::Type::TCP){
        ctx = modbus_new_tcp(config.ip.c_str(), config.port);
    }
    else{
        ctx = modbus_new_rtu(config.device.c_str(), config.baud,
                             config.parity, config.data_bit, config.stop_bit);
        modbus_set_slave(ctx, config.slave_id);
    }
    return ctx;
}
