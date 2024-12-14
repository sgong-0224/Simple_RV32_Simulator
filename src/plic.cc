#include "plic.h"
#include "cpu_exception.h"
#include "definitions.h"

#include <cstdint>
#include <cstdio>
#include <stdexcept>

uint32_t PLIC::read(uint32_t addr)
{
    if (addr >= PLIC_PRIV_BASE && addr < PLIC_PRIV_END)
        return priority[(addr-PLIC_PRIV_BASE)>>2];
    else if (addr >= PLIC_PEND_BASE && addr < PLIC_PEND_END)
        return read_pending(addr);
    else if (addr >= PLIC_EN_BASE && addr < PLIC_EN_END)
        return read_enable(addr);
    else if (addr == PLIC_THRESH )
        return thresh;
    else if (addr == PLIC_CLAIM ){
        // 读CLAIM清除pending, 并关闭该中断
        clear_pending(claim);
        clear_enable(claim);
        return claim;
    }else
        throw LoadAccessFault(addr);
}
void PLIC::write(uint32_t data,uint32_t addr)
{
    if (addr >= PLIC_PRIV_BASE && addr < PLIC_PRIV_END)
        priority[(addr-PLIC_PRIV_BASE)>>2]=data;
    else if (addr >= PLIC_PEND_BASE && addr < PLIC_PEND_END)
        return write_pending(data, addr);
    else if (addr >= PLIC_EN_BASE && addr < PLIC_EN_END)
        return write_enable(data, addr);
    else if (addr == PLIC_THRESH )
        thresh = data;
    else if (addr == PLIC_CLAIM ){
        set_enable(data);
        claim = data;
    }else
        throw StoreAccessFault(addr);
}
uint32_t PLIC::read_pending(uint32_t addr)
{
    std::shared_lock<std::shared_timed_mutex> lock(mtx);
    uint32_t index = (addr-PLIC_PEND_BASE)>>2;
    return pending[index];
}
uint32_t PLIC::read_enable(uint32_t addr)
{
    std::shared_lock<std::shared_timed_mutex> lock(mtx);
    uint32_t index = (addr-PLIC_EN_BASE)>>2;
    return enable[index];
}
void PLIC::write_pending(uint32_t data, uint32_t addr)
{
    std::unique_lock<std::shared_timed_mutex> lock(mtx);
    uint32_t index = (addr-PLIC_PEND_BASE)>>2;
    pending[index] = data;
}
void PLIC::write_enable(uint32_t data, uint32_t addr)
{
    std::unique_lock<std::shared_timed_mutex> lock(mtx);
    uint32_t index = (addr-PLIC_EN_BASE)>>2;
    enable[index] = data;
}
void PLIC::set_enable(uint32_t int_id)
{
    std::unique_lock<std::shared_timed_mutex> lock(mtx);
    uint32_t index = int_id >> 5;
    uint32_t bit = int_id % 32;
    enable[index] |= (1<<bit);
}
void PLIC::clear_enable(uint32_t int_id)
{
    std::unique_lock<std::shared_timed_mutex> lock(mtx);
    uint32_t index = int_id >> 5;
    uint32_t bit = int_id % 32;
    enable[index] &= ~(1<<bit);
}
bool PLIC::get_enable(uint32_t int_id)
{
    std::shared_lock<std::shared_timed_mutex> lock(mtx);
    uint32_t index = int_id >> 5;
    uint32_t bit = int_id % 32;
    return enable[index] & (1<<bit);
}
bool PLIC::get_pending(uint32_t int_id)
{
    std::shared_lock<std::shared_timed_mutex> lock(mtx);
    uint32_t index = int_id >> 5;
    uint32_t bit = int_id % 32;
    return pending[index] & (1<<bit);
}
void PLIC::set_pending(uint32_t int_id)
{
    std::unique_lock<std::shared_timed_mutex> lock(mtx);
    uint32_t index = int_id >> 5;
    uint32_t bit = int_id % 32;
    pending[index] |= (1<<bit);
}
void PLIC::clear_pending(uint32_t int_id)
{
    std::unique_lock<std::shared_timed_mutex> lock(mtx);
    uint32_t index = int_id >> 5;
    uint32_t bit = int_id % 32;
    pending[index] &= ~(1<<bit);
}
uint32_t PLIC::select_interrupt()
{
    std::map<uint32_t, uint32_t> int_priority;
    for(uint32_t i=1;i<priority.size();++i)
        if( priority[i]>=thresh && priority[i]!=0 && get_enable(i) && get_pending(i) )
            int_priority.insert({i,priority[i]});
    
    if(int_priority.empty())
        return 0;
    // 查找最大优先级以及对应的最小ID
    uint32_t max_priority = 0;
    uint32_t min_id_with_max_priority = 64;
    for (const auto& pair : int_priority) {
        if (pair.second > max_priority || 
            (pair.second == max_priority && pair.first < min_id_with_max_priority)) {
            max_priority = pair.second;
            min_id_with_max_priority = pair.first;
        }
    }
    // 设置CLAIM, 如果是合法的中断，写mip
    write(min_id_with_max_priority, PLIC_CLAIM);
    if(min_id_with_max_priority)
        csr.write(mip, csr.read(mip) | MASK_MEIP);
    return min_id_with_max_priority;
}