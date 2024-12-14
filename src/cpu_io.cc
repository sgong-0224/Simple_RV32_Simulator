#include "cpu_io.h"
#include "core.hpp"
#include "definitions.h"
#include "utils.hpp"
#include <cstdint>

void IO::pass_exception_to_handler(bool is_interrupt, uint8_t code, uint32_t address)
{
    core.get_exception_handler().handle(core, is_interrupt, code, address);
}

uint32_t IO::load(uint32_t addr, uint8_t width, bool exec)
{
    if(core.mode==USER_MODE){
        for(int i=0;i<16;++i){
            uint8_t pmp_bits = extract_bits(core.csr.read(pmpcfg+i/4), ((i%4)<<3)+7, (i%4)<<3);
            uint32_t addr_pmp = core.csr.read(pmpaddr+i);
            if( addr_pmp==addr && (pmp_bits&MASK_PMP_A)){           
                if((exec&&(!(pmp_bits&MASK_PMP_X)||!(pmp_bits&MASK_PMP_R))))     
                    pass_exception_to_handler(0, 1, addr);
                else if (!(pmp_bits&MASK_PMP_R))
                    pass_exception_to_handler(0, 5, addr);
                return -1;
            }
        }
    }
    if( addr>=MEM_BASE && addr<=MEM_END )
        return memory.load(addr-MEM_BASE, width);
    else if( addr>=UART_BASE && addr<=UART_END )
        return uart.read(addr-UART_BASE);
    else if( addr>=PLIC_BASE && addr<=PLIC_END )
        return plic.read(addr);
    else if( addr>=CLINT_BASE && addr<=MTIME_H )
        return clint.read(addr);
    else
        pass_exception_to_handler(0, 5, addr);
    // should not go here!
    return -1;
}

void IO::store(uint32_t data, uint32_t addr, uint8_t width)
{
    if(core.mode==USER_MODE){
        for(int i=0;i<16;++i){
            uint8_t pmp_bits = extract_bits(core.csr.read(pmpcfg+i/4), ((i%4)<<3)+7, (i%4)<<3);
            uint32_t addr_pmp = core.csr.read(pmpaddr+i);
            if( addr_pmp==addr && (pmp_bits&MASK_PMP_A) && !(pmp_bits&MASK_PMP_W)){                
                pass_exception_to_handler(0, 7, addr);
                return;
            }
        }
    }
    if( addr>=MEM_BASE && addr<=MEM_END )
        return memory.store(data, addr-MEM_BASE, width);
    else if( addr>=UART_BASE && addr<=UART_END )
        return uart.write(data,addr-UART_BASE);
    else if( addr>=PLIC_BASE && addr<=PLIC_END )
        return plic.write(data,addr);
    else if( addr>=CLINT_BASE && addr<=MTIME_CMP_H )
        return clint.write(data, addr);
    else
        pass_exception_to_handler(0, 7, addr);
}