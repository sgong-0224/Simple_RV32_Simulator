#include "cpu_exception.h"
#include "core.hpp"
#include "definitions.h"
#include <cstdint>
#include <optional>

void Exception::handle(Core& core, bool is_interrupt, uint8_t code, std::optional<uint32_t> info)
{
    core.reserved.clear();
    // 进入机器模式，写MPP
    uint8_t current_mode = core.mode;
    core.csr.write(mstatus, (core.csr.read(mstatus)&~MASK_MPP)|(current_mode<<11));
    core.mode = MACHINE_MODE;
    // 清除MIE, 写MPIE
    uint32_t mie_stat = core.csr.read(mstatus);
    uint32_t prev_mie = (mie_stat & MASK_MIE) >> 3;
    core.csr.write(mstatus, (mie_stat&~(MASK_MIE|MASK_MPIE))|(prev_mie<<7));
    // 写mcause, mtval
    core.csr.write(mcause,code|(is_interrupt<<31));
    if(!is_interrupt){
        if(info.has_value())
            core.csr.write(mtval, info.value());
        else
            core.csr.write(mtval,0);
    }
    // 保存PC
    core.csr.write(mepc, core.pc);
    core.pc = core.csr.read(mtvec);
    // 继续执行
    throw ContinueExec();
}