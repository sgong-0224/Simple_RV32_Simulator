#ifndef CPU_EXCEPTION_H
#define CPU_EXCEPTION_H
#include <cstdint>
#include <optional>

#include "definitions.h"
#include "utils.hpp"

class Core;
class Exception{
public:
    void handle(Core& core, bool is_interrupt, uint8_t code, std::optional<uint32_t> instruction);
};
class ContinueExec:public std::exception {
public:
    const char* what() const throw() {
        return "return to main control flow";
    }
};
class LoadAccessFault:public std::exception {
    uint32_t addr;
public:
    LoadAccessFault(uint32_t err_addr) { addr = err_addr; }
    uint32_t get_addr() { return addr; }
    const char* what() const throw() {
        return "LoadAccessFault!\n";
    }
};
class StoreAccessFault:public std::exception {
    uint32_t addr;
public:
    StoreAccessFault(uint32_t err_addr) { addr = err_addr; }
    uint32_t get_addr() { return addr; }
    const char* what() const throw() {
        return "StoreAccessFault!\n";
    }
};
#endif