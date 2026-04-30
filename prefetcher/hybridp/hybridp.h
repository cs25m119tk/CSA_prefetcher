#ifndef HYBRIDP_H
#define HYBRIDP_H

#include <cstdint>
#include "address.h"
#include "champsim.h"
#include "modules.h"

#define PT_SIZE 64

// thresholds
#define STRIDE_CONF 3
#define CORR_CONF 3

// limits
#define MAX_DEGREE 4
#define MIN_DEGREE 1
#define MAX_DISTANCE 8

struct PTEntry {
    bool valid = false;
    uint64_t pc = 0;

    // -------- stride --------
    uint64_t last_addr = 0;
    int64_t stride = 0;
    int stride_conf = 0;

    // -------- correlation --------
    uint64_t last_miss = 0;
    int64_t corr_delta = 0;
    int corr_conf = 0;

    // -------- dynamic Δ --------
    int trip_count = 0;
    int avg_trip = 1;

    // -------- adaptive control --------
    int degree = 2;
    int issued = 0;
    int useful = 0;
};

class hybridp : public champsim::modules::prefetcher {
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
    void update_trip(PTEntry& e);

    void adjust_degree(PTEntry& e);
    void issue_prefetch(PTEntry& e);
};

#endif