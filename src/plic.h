#ifndef PLIC_H
#define PLIC_H
#include <atomic>
#include <cstdint>
#include <optional>
#include <vector>
#include <map>
#include <mutex>
#include <shared_mutex>

#include "csr.hpp"
#include "definitions.h"

class UART;
class PLIC{
    friend class UART;
    CSR& csr;
    std::array<uint32_t,64> priority;
    std::array<uint32_t,2> pending;
    std::array<uint32_t,2> enable;
    uint32_t thresh;
    uint32_t claim;
    
public:
    std::shared_timed_mutex mtx;
    PLIC(CSR& csr): 
        csr(csr), thresh(0), claim(0){
            priority.fill(0);
            pending.fill(0);
            enable.fill(0);
        }
    
    uint32_t read(uint32_t addr);
    void write(uint32_t data, uint32_t addr);
    void write_enable(uint32_t data, uint32_t addr);
    uint32_t select_interrupt();
    
    uint32_t read_enable(uint32_t addr);
    bool get_enable(uint32_t int_id);
    void set_enable(uint32_t int_id);
    void clear_enable(uint32_t int_id);

    uint32_t read_pending(uint32_t addr);
    void write_pending(uint32_t data, uint32_t addr);
    bool get_pending(uint32_t int_id);
    void set_pending(uint32_t int_id);
    void clear_pending(uint32_t int_id);
};
#endif