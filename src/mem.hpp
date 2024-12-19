#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include "cpu_exception.h"
#include "definitions.h"

class Memory{
    std::vector<uint8_t> memory;
public:
    Memory(const std::string& binary_filename){
        std::fstream binary(binary_filename,std::ios::binary|std::ios::ate|std::ios::in);
        std::streamsize size = binary.tellg();
        binary.seekg(0, std::ios::beg);
        if( size>MEM_SIZE )
            throw std::out_of_range("Binary size exceeded memory size!\n");
        memory.resize(MEM_SIZE,0);
        binary.read(reinterpret_cast<char*>(memory.data()), size);
        if (binary.gcount() != size)
            throw std::runtime_error("Failed to read the expected size from the file.\n");
        binary.close();
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