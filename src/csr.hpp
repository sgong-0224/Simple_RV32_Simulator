#ifndef CSR_HPP
#define CSR_HPP
#include <cstdint>
#include <array>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <shared_mutex>

#include "utils.hpp"
#include "definitions.h"

class CLINT;
class PLIC;
class CSR{
    std::array<uint32_t, NUM_CSRS> csr;
    friend class CLINT;
    friend class PLIC;
    std::shared_timed_mutex mtx;
public:
    CSR() { csr.fill(0); }
    std::shared_timed_mutex& get_mutex() { return mtx; }
    uint32_t read(uint16_t addr);
    void write(uint16_t addr, uint32_t val);
    void dump_csrs();
};

inline uint32_t CSR::read(uint16_t addr)
{
    std::unique_lock<std::shared_timed_mutex> lock(mtx);
    switch (addr) {
        case mie:
            return csr[mie];
        case mip:
            return csr[mip];
        case mstatus:
            return csr[mstatus] & MASK_MSTATUS;
        default:
            return csr[addr];
    }
}

inline void CSR::write( uint16_t addr, uint32_t value)
{
    std::unique_lock<std::shared_timed_mutex> lock(mtx);
    if(addr==mie){
        csr[mie] = (csr[mie]&~MASK_MIP) | (value&MASK_MIP);
    }else if(addr==mip){
        csr[mip] = (csr[mip]&~MASK_MIP) | (value&MASK_MIP);
    }else if(addr==mstatus){
        csr[mstatus] = (csr[mstatus] & ~MASK_MSTATUS) | (value & MASK_MSTATUS);
    }else if(addr>=pmpaddr && addr<=pmpaddr+15){
        uint8_t offset = addr-pmpaddr;
        uint8_t config = extract_bits(csr[pmpcfg+offset/4],((offset%4)<<3)+7,offset%3);
        if(!(config&MASK_PMP_L))
            csr[addr] = value;
    }else{
        csr[addr] = value;
    }
}

inline void CSR::dump_csrs()
{
    std::unique_lock<std::shared_timed_mutex> lock(mtx);
    std::cout << std::setfill('0')
              << "mstatus = "  << std::hex << std::setw(8) << read(mstatus)
              << "  mtvec = "  << std::hex << std::setw(8) << read(mtvec)
              << "  mepc = "   << std::hex << std::setw(8) << read(mepc)
              << "  mcause = " << std::hex << std::setw(8) << read(mcause) << "\n"
              << "mscratch = "  << std::hex << std::setw(8) << read(mscratch)
              << "  mtval = "  << std::hex << std::setw(8) << read(mtval) 
              << "  mie = "  << std::hex << std::setw(8) << read(mie)
              << "  mip = "  << std::hex << std::setw(8) << read(mip) << '\n';
}
#endif