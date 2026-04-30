#ifndef IMP_H
#define IMP_H

#include <cstdint>
#include "address.h"
#include "champsim.h"
#include "modules.h"

#define PT_SIZE 32
#define IPD_SIZE 16
#define CONF_THRESHOLD 3
#define MAX_DISTANCE 3

struct PTEntry {
    bool valid = false;
    uint64_t pc = 0;

    uint64_t last_stream_addr = 0;

    bool indirect = false;
    int64_t delta = 0;
    int confidence = 0;
};

struct IPDEntry {
    bool valid = false;
    uint64_t pc = 0;

    uint64_t last_stream = 0;
    int64_t last_delta = 0;
    int confidence = 0;
};

class imp : public champsim::modules::prefetcher {
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
    PTEntry pt[PT_SIZE];
    IPDEntry ipd[IPD_SIZE];

    int find_pt(uint64_t pc);
    int alloc_pt(uint64_t pc);

    int find_ipd(uint64_t pc);
    int alloc_ipd(uint64_t pc);

    void update_ipd(IPDEntry& e, uint64_t stream_addr, uint64_t miss_addr);
    void issue_prefetch(PTEntry& e);
};

#endif