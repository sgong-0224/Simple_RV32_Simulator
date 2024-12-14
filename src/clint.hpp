#ifndef CLINT_HPP
#define CLINT_HPP
#include <chrono>
#include <cstdint>
#include <thread>
#include <atomic>
#include <stdexcept>

#include "definitions.h"
#include "csr.hpp"

class CSR;
class CLINT{
    CSR& csr;
    std::chrono::steady_clock::time_point start_time;
    std::atomic<uint64_t> mtime;
    std::atomic<uint64_t> mtimecmp;
    std::atomic<bool> updated_L_mcmp;
    std::atomic<bool> updated_H_mcmp;
    std::atomic<bool> interrupt;
    std::thread update;
    bool running;

public:
    CLINT(CSR& csr): csr(csr),running(true), interrupt(false),
                     updated_L_mcmp(0),updated_H_mcmp(0) 
    {
        reset();
        update = std::thread([this]() { this->update_tick(); });
    }
    ~CLINT() {
        running = false;
        if (update.joinable())
            update.join();
    }
    void reset() { 
        start_time = std::chrono::steady_clock::now();
        mtime.store(0); 
    }
    void update_tick(){
        while (running) {
            auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - start_time
            ).count();
            mtime = static_cast<uint64_t>(elapsed/100);
            if( mtime>=mtimecmp && (updated_H_mcmp&updated_L_mcmp) && !interrupt ){
                csr.write(mip, csr.read(mip)|MASK_MTIP);
                updated_H_mcmp = updated_L_mcmp = 0;
                interrupt = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    void complete_interrupt() { interrupt = false; }
    uint32_t read(uint32_t addr);
    void write(uint32_t data, uint32_t addr);
};

inline uint32_t CLINT::read(uint32_t addr)
{
    switch (addr) {
        case MTIME_CMP_H:
            return (mtimecmp&0xffff'ffff'0000'0000)>>32;
        case MTIME_CMP_L:
            return mtimecmp&0x0000'0000'ffff'ffff;
        case MTIME_H:
            return (mtime&0xffff'ffff'0000'0000)>>32;
        case MTIME_L:
            return mtime&0x0000'0000'ffff'ffff;
        default:
            throw std::runtime_error("CLINT address out of range!\n");
    }
}

inline void CLINT::write(uint32_t data, uint32_t addr)
{
    switch (addr) {
        case MTIME_CMP_H:
            mtimecmp = (mtimecmp&0x0000'0000'ffff'ffff)|(static_cast<uint64_t>(data)<<32);
            updated_H_mcmp = 1;
            break;
        case MTIME_CMP_L:
            mtimecmp = (mtimecmp&0xffff'ffff'0000'0000)|data;
            updated_L_mcmp = 1;
            break;
        default:
            throw std::runtime_error("CLINT store violation!\n");
    }
}
#endif