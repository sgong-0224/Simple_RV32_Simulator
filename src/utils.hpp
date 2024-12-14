#ifndef UTILS_HPP
#define UTILS_HPP
#include <cstdint>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>


/* ======================= INSTRUCTION DECODE ========================*/
inline uint32_t extract_bits(uint32_t value, uint8_t high, uint8_t low) 
{
    uint32_t mask = ((1u << (high - low + 1)) - 1) << low;
    return (value & mask) >> low;
}
inline uint32_t extend_to_u32(uint32_t value, uint8_t original_bits) 
{
    bool negative = (value & (1u << (original_bits - 1)));
    if (negative) {
        uint32_t mask = ~((1u << original_bits) - 1);
        value |= mask;
    }
    return value;
}
/* ======================== CSR ADDR UTILITY =========================*/
inline uint8_t min_priv(uint16_t addr)
{
    return extract_bits(addr, 9, 8);
}
inline bool is_readonly(uint16_t addr)
{
    return extract_bits(addr, 11, 10)==0b11;
}
/* ======================== TERMINAL UTILITY =========================*/
static pthread_mutex_t stdin_mutex = PTHREAD_MUTEX_INITIALIZER;
inline struct termios original_tio;
inline void reset_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

inline void disable_buffered_input() {
    struct termios tio;
    tcgetattr(STDIN_FILENO, &tio);
    original_tio = tio;
    tio.c_lflag &= ~(ICANON | ECHO);
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &tio);
    atexit(reset_terminal_mode);
}

inline int8_t nonblocking_getchar() {
    char ch;
    pthread_mutex_lock(&stdin_mutex);
    read(STDIN_FILENO, &ch, 1);
    pthread_mutex_unlock(&stdin_mutex);
    return ch>0 ? ch:-1;
}
#endif