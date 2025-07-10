#pragma once

#include <cstdint>
#include <limits>

#ifndef CROSSBAR_SIZE
#define CROSSBAR_SIZE 32
#endif

#ifndef BIT_PRECISION
#define BIT_PRECISION 1
#endif

constexpr uint32_t ACC_SIZE = CROSSBAR_SIZE;
constexpr uint32_t ACT_SIZE = CROSSBAR_SIZE;  
constexpr uint32_t UNIT_TIME = 1;
constexpr uint32_t UNIT_ADDR = 0x10;

/* Bandwidth */
#ifndef BANDWIDTH
#define BANDWIDTH 2048*2048
#endif

constexpr uint32_t HOST_BW = std::numeric_limits<uint32_t>::max();

// Network Bandwidth
constexpr uint32_t CB_ACC_BW  = BANDWIDTH;
constexpr uint32_t ACC_ACT_BW = BANDWIDTH;
constexpr uint32_t ACT_CB_BW  = BANDWIDTH;

constexpr uint32_t IM_CB_BW = BANDWIDTH;

constexpr uint32_t LAYER_BW = BANDWIDTH;

// Component Bandwidth
constexpr uint32_t CB_IN_BW        = CROSSBAR_SIZE; 
constexpr uint32_t CB_OUT_BW       = CROSSBAR_SIZE; 
constexpr uint32_t ACC_IN_BW       = ACC_SIZE; 
constexpr uint32_t ACC_OUT_BW      = ACC_SIZE; 
constexpr uint32_t ACT_IN_BW       = ACT_SIZE; 
constexpr uint32_t ACT_OUT_BW      = ACT_SIZE; 
constexpr uint32_t IM_IN_BW        = BANDWIDTH;
constexpr uint32_t IM_OUT_BW       = BANDWIDTH;
constexpr uint32_t POOL_IN_BW      = BANDWIDTH;
constexpr uint32_t POOL_OUT_BW     = BANDWIDTH;
constexpr uint32_t FLATTEN_IN_BW   = BANDWIDTH;
constexpr uint32_t FLATTEN_OUT_BW  = BANDWIDTH;