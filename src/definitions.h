#ifndef DEFINITIONS_H
#define DEFINITIONS_H
#include <cstdint>
/* ========================== CONFIG ==========================*/


/* ========================== MEMORY ==========================*/
constexpr uint32_t  MEM_SIZE = 128 << 20;
constexpr uint32_t  MEM_BASE = 0x80000000;
constexpr uint32_t  MEM_END  = MEM_BASE + MEM_SIZE - 1;

/* =========================== MMIO ===========================*/
// clint
constexpr uint32_t CLINT_BASE  = 0x02000000;
constexpr uint32_t MTIME_L     = CLINT_BASE + 0xbff8;
constexpr uint32_t MTIME_H     = CLINT_BASE + 0xbffc;
constexpr uint32_t MTIME_CMP_L = CLINT_BASE + 0x4000;
constexpr uint32_t MTIME_CMP_H = CLINT_BASE + 0x4004;

// uart
constexpr uint32_t UART_BASE = 0x10000000;
constexpr uint32_t UART_END  = UART_BASE + 7;
constexpr uint32_t UART_IRQ  = 10;
constexpr uint8_t  UART_THR  = 0;
constexpr uint8_t  UART_RHR  = 0;
constexpr uint8_t  UART_LSR  = 5;
constexpr uint8_t  MASK_UART_LSR_RX = 1;
constexpr uint8_t  MASK_UART_LSR_TX = 1 << 5;
// plic
constexpr uint32_t PLIC_BASE        = 0x0c000000;
constexpr uint32_t PLIC_SIZE        = 0x04000000;
constexpr uint32_t PLIC_PRIV_BASE   = PLIC_BASE;
constexpr uint32_t PLIC_PRIV_END    = PLIC_BASE + 0x0040;
constexpr uint32_t PLIC_PEND_BASE   = PLIC_BASE + 0x1000;
constexpr uint32_t PLIC_PEND_END    = PLIC_BASE + 0x1002;
constexpr uint32_t PLIC_EN_BASE     = PLIC_BASE + 0x2000;
constexpr uint32_t PLIC_EN_END      = PLIC_BASE + 0x2002;
constexpr uint32_t PLIC_THRESH      = PLIC_BASE + 0x200000;
constexpr uint32_t PLIC_CLAIM       = PLIC_BASE + 0x200004;
constexpr uint32_t PLIC_END         = PLIC_BASE + PLIC_SIZE -1;

/* ======================== PRIVILIGE =========================*/
// S Mode unimplemented
constexpr uint8_t USER_MODE    = 0b00;
constexpr uint8_t MACHINE_MODE = 0b11;

/* ======================= CSR ADDRESS ========================*/
constexpr uint16_t NUM_CSRS   = 4096;
// time
constexpr uint16_t time_csr   = 0xc01;
// machine info
constexpr uint16_t mhartid    = 0xf14;
// machine trap
constexpr uint16_t mstatus    = 0x300;
constexpr uint16_t mie        = 0x304;
constexpr uint16_t mtvec      = 0x305;
constexpr uint16_t mcounteren = 0x306;
constexpr uint16_t mscratch   = 0x340;
constexpr uint16_t mepc       = 0x341;
constexpr uint16_t mcause     = 0x342;
constexpr uint16_t mtval      = 0x343;
constexpr uint16_t mip        = 0x344;
// PMP & MASK
constexpr uint16_t pmpcfg     = 0x3a0;  // 0~3
constexpr uint16_t pmpaddr    = 0x3b0;  // 0~15
constexpr uint8_t  MASK_PMP_R = 1<<0;
constexpr uint8_t  MASK_PMP_W = 1<<1;
constexpr uint8_t  MASK_PMP_X = 1<<2;
constexpr uint8_t  MASK_PMP_A = 0b11<<3;
constexpr uint8_t  MASK_PMP_L = 1<<7;

// mstatus masks
constexpr uint32_t MASK_MIE   = 1 << 3;
constexpr uint32_t MASK_MPIE  = 1 << 7;  
constexpr uint32_t MASK_MPP   = 0b11 << 11;  
constexpr uint32_t MASK_MPRV  = 1 << 17;  
constexpr uint32_t MASK_MSTATUS = MASK_MIE | MASK_MPIE | MASK_MPP | MASK_MPRV;
// mie/mip masks
constexpr uint32_t MASK_MSIP = 1 << 3;  
constexpr uint32_t MASK_MTIP = 1 << 7;  
constexpr uint32_t MASK_MEIP = 1 << 11; 
constexpr uint32_t MASK_MIP  = MASK_MEIP|MASK_MSIP|MASK_MTIP; 

#endif