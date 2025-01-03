# ifndef CORE_HPP
# define CORE_HPP

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <functional>
#include <stdexcept>
#include <set>
#include <iostream>
#include <iomanip>
#include <optional>

#include "cpu_io.h"
#include "cpu_exception.h"
#include "csr.hpp"
#include "definitions.h"
#include "utils.hpp"

class Core {
    // control
    uint8_t mode = MACHINE_MODE;

    // registers
    uint32_t pc = 0;
    std::array<uint32_t,32> regfile;
    const std::array<std::string, 32> reg_alias = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
    };
    CSR csr;

    // atomic
    std::set<uint32_t> reserved;

    // execution
    std::unordered_map<uint8_t, std::function<void(uint32_t)>> instr_exec_func;
    void exec_load(uint32_t instruction);
    void exec_fence(uint32_t instruction);
    void exec_alg_logic_imm(uint32_t instruction);
    void exec_auipc(uint32_t instruction);
    void exec_store(uint32_t instruction);
    void exec_atomic(uint32_t instruction);
    void exec_alg_logic(uint32_t instruction);
    void exec_lui(uint32_t instruction);
    void exec_branch(uint32_t instruction);
    void exec_jalr(uint32_t instruction);
    void exec_jal(uint32_t instruction);
    void exec_system(uint32_t instruction);
    void exec_illegal(uint32_t instruction);

    // external
    friend class IO;
    IO cpu_io;
    // exception&interrupt
    friend class Exception;
    Exception exception_handler;

public:
    Core(const std::string& code_filename):cpu_io(*this, code_filename, csr){
        instr_exec_func[0b0000011] = [this](uint32_t instruction){this->exec_load(instruction);};
        instr_exec_func[0b0001111] = [this](uint32_t instruction){this->exec_fence(instruction);};
        instr_exec_func[0b0010011] = [this](uint32_t instruction){this->exec_alg_logic_imm(instruction);};
        instr_exec_func[0b0010111] = [this](uint32_t instruction){this->exec_auipc(instruction);};
        instr_exec_func[0b0100011] = [this](uint32_t instruction){this->exec_store(instruction);};
        instr_exec_func[0b0101111] = [this](uint32_t instruction){this->exec_atomic(instruction);};
        instr_exec_func[0b0110011] = [this](uint32_t instruction){this->exec_alg_logic(instruction);};
        instr_exec_func[0b0110111] = [this](uint32_t instruction){this->exec_lui(instruction);};
        instr_exec_func[0b1100011] = [this](uint32_t instruction){this->exec_branch(instruction);};
        instr_exec_func[0b1100111] = [this](uint32_t instruction){this->exec_jalr(instruction);};
        instr_exec_func[0b1101111] = [this](uint32_t instruction){this->exec_jal(instruction);};
        instr_exec_func[0b1110011] = [this](uint32_t instruction){this->exec_system(instruction);};
        
        init_regs();
        init_mode();
        init_stack();
        init_pc();
    }
    // exception/interrupt
    Exception& get_exception_handler() { return exception_handler; }
    std::optional<uint32_t> check_pending_interrupt();
    void check_and_handle_interrupts();
    uint32_t current_irq = 0;
    bool handling_timer_int = 0;

    void init_pc()  { pc = MEM_BASE; }
    void init_mode() { mode = MACHINE_MODE; }
    void init_stack() { regfile[2] = MEM_END; }
    void init_regs() { regfile.fill(0); }

    void execute(uint32_t instruction);
    std::optional<uint32_t> fetch();
    
    // debug
    void dump_registers();
};


inline void Core::check_and_handle_interrupts()
{
    std::optional<uint32_t> int_type = check_pending_interrupt();
    if(int_type.has_value())
        exception_handler.handle(*this, 1, int_type.value(), std::nullopt);
}

inline std::optional<uint32_t> Core::check_pending_interrupt() 
{
    if ((this->mode == MACHINE_MODE) && !(this->csr.read(mstatus)&MASK_MIE))
        return std::nullopt;
    
    auto &mutex = csr.get_mutex();

    // 时钟中断
    uint32_t pending = this->csr.read(mie) & this->csr.read(mip);
    if (pending & MASK_MTIP) { 
        handling_timer_int = 1;
        this->csr.write(mip, this->csr.read(mip) & ~MASK_MTIP);
        pending = this->csr.read(mie) & this->csr.read(mip);
        return 7;
    }

    // 选择待处理的外部中断
    uint32_t int_id = this->cpu_io.select_avail_int();
    if(int_id){
        pending = this->csr.read(mie) & this->csr.read(mip);
        if (pending & MASK_MEIP) {
            current_irq = cpu_io.load(PLIC_CLAIM,32);
            this->csr.write(mip, this->csr.read(mip) & ~MASK_MEIP);
            return 11;
        }
    }
    
    // 软件中断
    pending = this->csr.read(mie) & this->csr.read(mip);
    if (pending & MASK_MSIP) {
        this->csr.write(mip, this->csr.read(mip) & ~MASK_MSIP);
        return 3;
    }else{
        return std::nullopt;
    }
}

// fetch & execute
inline std::optional<uint32_t> Core::fetch()
{
    if((pc&0b11)!=0){
        exception_handler.handle(*this, 0, 0, pc);
        return std::nullopt;
    }
    uint32_t inst = 0;
    try{
        inst = cpu_io.load(pc,32,true);
    }catch (std::exception& e){
        exception_handler.handle(*this, 0, 1, pc);
        return std::nullopt;
    }
    return inst;
}

inline void Core::execute(uint32_t instruction)
{
    uint8_t opcode = static_cast<uint8_t>(extract_bits(instruction, 6, 0));
    try{
        instr_exec_func.at(opcode)(instruction);
    }catch(const std::out_of_range& illegal_inst){
        exec_illegal(instruction);
        return;
    }
    regfile[0] = 0;   
}

// debug
inline void Core::dump_registers() 
{
    std::cout << std::setw(80) << std::setfill('-') << "" << '\n';
    std::cout << "PC = " << std::hex << std::setw(8) << std::setfill('0') << pc << '\n'; 
    std::cout << std::setfill(' ');
    for (size_t i = 0; i < 32; i += 4) {
        std::cout << std::setfill('0')
                  << "x" << std::dec << std::setw(2)  << i << "(" << reg_alias[i] << ") = " 
                  << std::setw(8) << std::hex << regfile[i] << " "
                  << "x" << std::dec << std::setw(2)  << i+1 << "(" << reg_alias[i+1] << ") = " 
                  << std::setw(8) << std::hex << regfile[i+1] << " "
                  << "x" << std::dec << std::setw(2)  << i+2 << "(" << reg_alias[i+2] << ") = " 
                  << std::setw(8) << std::hex << regfile[i+2] << " "
                  << "x" << std::dec << std::setw(2)<< i+3 << "(" << reg_alias[i+3] << ") = " 
                  << std::setw(8) << std::hex << regfile[i+3] << '\n';
    }
    csr.dump_csrs();
}

#endif