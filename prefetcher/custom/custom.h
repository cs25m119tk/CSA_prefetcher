#ifndef CUSTOM_H
#define CUSTOM_H

#include <cstdint>
#include "address.h"
#include "champsim.h"
#include "modules.h"

#define PT_SIZE 64

// thresholds
#define STRIDE_CONF 4
#define CORR_CONF 3
#define IND_CONF 3

// limits
#define MAX_DEGREE 6
#define MIN_DEGREE 1
#define MAX_DISTANCE 16

struct PTEntry {
    bool valid = false;
    uint64_t pc = 0;

    // stride
    uint64_t last_addr = 0;
    int64_t stride = 0;
    int stride_conf = 0;

    // correlation (delta)
    uint64_t last_addr2 = 0;
    int64_t last_delta = 0;
    int64_t corr_delta = 0;
    int corr_conf = 0;

    // indirect-lite
    int ind_conf = 0;

    // dynamic lookahead
    int distance = 2;

    // control
    int degree = 2;
    int issued = 0;
    int useful = 0;
};

class custom : public champsim::modules::prefetcher {
public:
    using champsim::modules::prefetcher::prefetcher;

    uint32_t prefetcher_cache_operate(
        champsim::address addr,
        champsim::address ip,
        uint8_t cache_hit,
        bool useful_prefetch,
        access_type type,
        uint32_t metadata_in
    );

    uint32_t prefetcher_cache_fill(
        champsim::address addr,
        long set,
        long way,
        uint8_t prefetch,
        champsim::address evicted_addr,
        uint32_t metadata_in
    );

private:
    PTEntry table[PT_SIZE];

    int find(uint64_t pc);
    int alloc(uint64_t pc);

    void update_stride(PTEntry& e, uint64_t addr);
    void update_correlation(PTEntry& e, uint64_t addr);
    void adjust_degree(PTEntry& e);
    void update_distance(PTEntry& e);

    void issue_prefetch(PTEntry& e);
};

#endif