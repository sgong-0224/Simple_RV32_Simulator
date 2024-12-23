#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <array>
#include <mutex>
#include <thread>

#include "definitions.h"
#include "utils.hpp"
#include "plic.h"

class UART{
    std::array<int8_t,8> uart_regs; 
    PLIC& plic;
    std::atomic<bool> should_stop;
    std::thread input_thread;
public:
    UART(PLIC& plic):plic(plic){ 
        uart_regs.fill(0); 
        // 初始化RHR为-1，LSR_TX为1，LSR_RX为0
        uart_regs[UART_RHR] = ~0;
        uart_regs[UART_LSR] |=  MASK_UART_LSR_TX;
        uart_regs[UART_LSR] &= ~MASK_UART_LSR_RX;
        // 输入线程
        input_thread = std::thread([&]() {
            while(!should_stop) {
                int8_t byte = nonblocking_getchar();
                if(byte==-1){
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;    
                }
                {
                    std::unique_lock<std::mutex> lock(mtx);
                    uart_regs[UART_RHR] = byte;
                    plic.set_pending(UART_IRQ);
                    // 发送置位
                    uart_regs[UART_LSR] |= MASK_UART_LSR_RX;
                    cv.notify_all();
                }
            }
        });
    } 
    ~UART() {
        should_stop.store(true);
        if (input_thread.joinable())
            input_thread.join();
    }
    int32_t read(int8_t reg);
    void write(int8_t data, int8_t reg);
    // concurrency
    std::condition_variable cv;
    std::mutex mtx;
};

inline int32_t UART::read(int8_t reg)
{
    if(reg>7) 
        return -1;
    std::unique_lock<std::mutex> lock(mtx);
    if(reg==UART_RHR){
        cv.wait(lock, [this]() { 
            return (uart_regs[UART_LSR]&MASK_UART_LSR_RX); 
        });
        if (uart_regs[UART_LSR] & MASK_UART_LSR_RX) {
            // 接收置位
            uart_regs[UART_LSR] &= ~MASK_UART_LSR_RX;
            return uart_regs[UART_RHR];
        }
        return -1;
    }else{
        return uart_regs[reg];
    }
}

inline void UART::write(int8_t data, int8_t reg)
{
    if(reg>7) 
        return;
    std::unique_lock<std::mutex> lock(mtx);
    if(reg==UART_THR){
        cv.wait(lock, [this]() { 
            return (uart_regs[UART_LSR] & MASK_UART_LSR_TX); 
        });
        uart_regs[UART_LSR]&=~MASK_UART_LSR_TX;
        if(data!=-1)
            putchar(static_cast<char>(data));
        uart_regs[UART_LSR]|=MASK_UART_LSR_TX;
    }else{
        uart_regs[reg] = data;
    }
}