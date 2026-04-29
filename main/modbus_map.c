#include "modbus_map.h"
static uint16_t regs[16]; uint16_t modbus_map_read(uint16_t r){return (r<16)?regs[r]:0;} void modbus_map_write(uint16_t r,uint16_t v){if(r<16)regs[r]=v;}
