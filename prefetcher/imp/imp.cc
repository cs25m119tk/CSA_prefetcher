#include "imp.h"
#include "cache.h"

// ------------------------------
int imp::find_pt(uint64_t pc) {
    for (int i = 0; i < PT_SIZE; i++)
        if (pt[i].valid && pt[i].pc == pc)
            return i;
    return -1;
}

int imp::alloc_pt(uint64_t pc) {
    for (int i = 0; i < PT_SIZE; i++) {
        if (!pt[i].valid) {
            pt[i] = {};
            pt[i].valid = true;
            pt[i].pc = pc;
            return i;
        }
    }
    return pc % PT_SIZE;
}

// ------------------------------
int imp::find_ipd(uint64_t pc) {
    for (int i = 0; i < IPD_SIZE; i++)
        if (ipd[i].valid && ipd[i].pc == pc)
            return i;
    return -1;
}

int imp::alloc_ipd(uint64_t pc) {
    for (int i = 0; i < IPD_SIZE; i++) {
        if (!ipd[i].valid) {
            ipd[i] = {};
            ipd[i].valid = true;
            ipd[i].pc = pc;
            return i;
        }
    }
    return pc % IPD_SIZE;
}

// ------------------------------
// CORE: IPD learning
void imp::update_ipd(IPDEntry& e,
                     uint64_t stream_addr,
                     uint64_t miss_addr) {

    int64_t delta = (int64_t)(miss_addr - stream_addr);

    if (delta == e.last_delta)
        e.confidence++;
    else {
        e.last_delta = delta;
        e.confidence = 1;
    }

    e.last_stream = stream_addr;
}

// ------------------------------
void imp::issue_prefetch(PTEntry& e) {

    if (!e.indirect || e.confidence < CONF_THRESHOLD)
        return;

    for (int i = 1; i <= MAX_DISTANCE; i++) {

        uint64_t pf_addr = e.last_stream_addr + e.delta * i;

        // page boundary protection
        if ((pf_addr >> 12) != (e.last_stream_addr >> 12))
            break;

        pf_addr = (pf_addr >> 6) << 6;

        intern_->prefetch_line(pf_addr, true, 0);
    }
}

// ------------------------------
uint32_t imp::prefetcher_cache_operate(
    champsim::address addr,
    champsim::address ip,
    uint8_t cache_hit,
    bool,
    access_type,
    uint32_t metadata_in
) {
    uint64_t a = addr.to<uint64_t>();
    uint64_t pc = ip.to<uint64_t>();

    int p = find_pt(pc);
    if (p < 0) p = alloc_pt(pc);

    int i = find_ipd(pc);
    if (i < 0) i = alloc_ipd(pc);

    PTEntry& pe = pt[p];
    IPDEntry& ie = ipd[i];

    // --------- LEARNING ONLY ON MISSES ----------
    if (cache_hit == 0 && pe.last_stream_addr != 0) {

        update_ipd(ie, pe.last_stream_addr, a);

        if (ie.confidence >= CONF_THRESHOLD) {
            pe.indirect = true;
            pe.delta = ie.last_delta;
            pe.confidence = ie.confidence;
        }
    }

    pe.last_stream_addr = a;

    issue_prefetch(pe);

    return metadata_in;
}

// ------------------------------
uint32_t imp::prefetcher_cache_fill(
    champsim::address,
    long,
    long,
    uint8_t,
    champsim::address,
    uint32_t metadata_in
) {
    return metadata_in;
}