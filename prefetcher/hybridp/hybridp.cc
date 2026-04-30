#include "hybridp.h"
#include "cache.h"

// -------------------------------
int hybridp::find(uint64_t pc) {
    for (int i = 0; i < PT_SIZE; i++)
        if (table[i].valid && table[i].pc == pc)
            return i;
    return -1;
}

int hybridp::alloc(uint64_t pc) {
    for (int i = 0; i < PT_SIZE; i++) {
        if (!table[i].valid) {
            table[i] = {};
            table[i].valid = true;
            table[i].pc = pc;
            return i;
        }
    }
    return pc % PT_SIZE;
}

// -------------------------------
void hybridp::update_stride(PTEntry& e, uint64_t addr) {
    if (e.last_addr != 0) {
        int64_t s = (int64_t)(addr - e.last_addr);

        if (s == e.stride)
            e.stride_conf++;
        else {
            e.stride = s;
            e.stride_conf = 1;
        }
    }
    e.last_addr = addr;
}

// -------------------------------
void hybridp::update_correlation(PTEntry& e, uint64_t addr) {

    if (e.last_miss != 0) {
        int64_t d = (int64_t)(addr - e.last_miss);

        if (d == e.corr_delta)
            e.corr_conf++;
        else {
            e.corr_delta = d;
            e.corr_conf = 1;
        }
    }

    e.last_miss = addr;
}

// -------------------------------
void hybridp::update_trip(PTEntry& e) {

    e.trip_count++;

    // detect reset (simple heuristic)
    if (e.trip_count > 32) {
        e.avg_trip = (e.avg_trip + e.trip_count) / 2;
        e.trip_count = 0;
    }
}

// -------------------------------
void hybridp::adjust_degree(PTEntry& e) {

    if (e.issued < 20)
        return;

    float acc = (float)e.useful / (float)e.issued;

    if (acc < 0.2 && e.degree > MIN_DEGREE)
        e.degree--;

    else if (acc > 0.5 && e.degree < MAX_DEGREE)
        e.degree++;

    e.issued = 0;
    e.useful = 0;
}

// -------------------------------
void hybridp::issue_prefetch(PTEntry& e) {

    int delta = 1;

    if (e.avg_trip > 1)
        delta = std::min(MAX_DISTANCE, e.avg_trip / 2);

    // -------- STRIDE MODE --------
    if (e.stride_conf >= STRIDE_CONF) {

        for (int i = 1; i <= e.degree; i++) {

            uint64_t pf = e.last_addr + e.stride * i * delta;

            if ((pf >> 12) != (e.last_addr >> 12))
                break;

            pf = (pf >> 6) << 6;

            if (intern_->prefetch_line(pf, true, 0))
                e.issued++;
        }
    }

    // -------- CORRELATION MODE --------
    else if (e.corr_conf >= CORR_CONF) {

        for (int i = 1; i <= e.degree; i++) {

            uint64_t pf = e.last_addr + e.corr_delta * i * delta;

            if ((pf >> 12) != (e.last_addr >> 12))
                break;

            pf = (pf >> 6) << 6;

            if (intern_->prefetch_line(pf, true, 0))
                e.issued++;
        }
    }
}

// -------------------------------
uint32_t hybridp::prefetcher_cache_operate(
    champsim::address addr,
    champsim::address ip,
    uint8_t cache_hit,
    bool useful_prefetch,
    access_type,
    uint32_t metadata_in
) {
    uint64_t a = addr.to<uint64_t>();
    uint64_t pc = ip.to<uint64_t>();

    int idx = find(pc);
    if (idx < 0) idx = alloc(pc);

    PTEntry& e = table[idx];

    // update structures
    update_stride(e, a);
    update_trip(e);

    if (!cache_hit)
        update_correlation(e, a);

    // useful tracking
    if (useful_prefetch)
        e.useful++;

    adjust_degree(e);

    issue_prefetch(e);

    return metadata_in;
}

// -------------------------------
uint32_t hybridp::prefetcher_cache_fill(
    champsim::address,
    long,
    long,
    uint8_t,
    champsim::address,
    uint32_t metadata_in
) {
    return metadata_in;
}