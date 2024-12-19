#ifndef CPU_IO_H
#define CPU_IO_H

#include <cstdint>
#include <shared_mutex>
#include <stdexcept>
#include <string>

#include "mem.hpp"
#include "uart.hpp"
#include "plic.h"
#include "clint.hpp"
#include "csr.hpp"
#include "definitions.h"

class Core;
class CSR;
class IO{
    Core&  core;
    // devices
    Memory memory;
    UART   uart;
    PLIC   plic;
    CLINT  clint;

public:
    IO(Core& core, const std::string& code_filename, CSR& csr):
        core(core),memory(code_filename),plic(csr),uart(plic),clint(csr){}
    // access    
    uint32_t load(uint32_t addr, uint8_t width, bool exec=false);
    void store(uint32_t data, uint32_t addr, uint8_t width);
    // exception handler
    void pass_exception_to_handler(bool is_interrupt, uint8_t code, uint32_t address);
    // plic
    uint32_t select_avail_int(){ return plic.select_interrupt(); }
    void clear_int_pending(uint32_t id){ plic.clear_pending(id); }
    std::shared_timed_mutex& get_plic_mutex(){ return plic.mtx; }
    // clint
    void clear_clint_int(){ clint.complete_interrupt(); }
};
#endif