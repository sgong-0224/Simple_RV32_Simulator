#include <cstdint>
#include <vector>

#include "cpu_exception.h"
#include "definitions.h"

class Memory{
    std::vector<uint8_t> memory;
public:
    Memory(const std::vector<uint8_t>& binary){
        memory.resize(MEM_SIZE,0);
        std::copy(binary.begin(),binary.end(),memory.begin());
    }
    uint32_t load(uint32_t addr, uint8_t width);
    void store(uint32_t data, uint32_t addr, uint8_t width);
};

inline uint32_t Memory::load(uint32_t addr, uint8_t width)
{
    if( width!=8 && width!=16 && width!=32 )
        throw LoadAccessFault(addr);
    uint8_t  nbytes = width/8;
    if( addr+nbytes > memory.size() )
        throw LoadAccessFault(addr);
    uint32_t data = 0;
    for(uint8_t i=0;i<nbytes;++i)
        data |= static_cast<uint32_t>( memory[addr+i]<<(i<<3) );
    return data;
}

inline void Memory::store(uint32_t data, uint32_t addr, uint8_t width)
{
    if( width!=8 && width!=16 && width!=32 )
        throw StoreAccessFault(addr);
    uint8_t  nbytes = width/8;
    if( addr+nbytes > memory.size() )
        throw StoreAccessFault(addr);
    for(uint8_t i=0;i<nbytes;++i)
        memory[addr+i] = (data >> (i<<3)) & 0xFF;
}