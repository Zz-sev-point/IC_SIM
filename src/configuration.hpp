#pragma once

#ifndef CROSSBAR_SIZE
#define CROSSBAR_SIZE 32
#endif

#ifndef BIT_PRECISION
#define BIT_PRECISION 1
#endif

#ifndef BANDWIDTH
#define BANDWIDTH 256
#endif

constexpr uint32_t ACC_SIZE = CROSSBAR_SIZE;
constexpr uint32_t ACT_SIZE = CROSSBAR_SIZE;  
constexpr uint32_t UNIT_TIME = 1;
constexpr uint32_t UNIT_ADDR = 0x10;