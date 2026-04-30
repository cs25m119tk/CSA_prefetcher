#include "custom.h"
#include "cache.h"

// -------------------------------
int custom::find(uint64_t pc) {
    for (int i = 0; i < PT_SIZE; i++)
        if (table[i].valid && table[i].pc == pc)
            return i;
    return -1;
}

int custom::alloc(uint64_t pc) {
    int victim = pc % PT_SIZE;
    table[victim] = {};
    table[victim].valid = true;
    table[victim].pc = pc;
    return victim;
}

// -------------------------------
void custom::update_stride(PTEntry& e, uint64_t addr) {
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
void custom::update_correlation(PTEntry& e, uint64_t addr) {

    if (e.last_addr2 != 0) {
        int64_t delta = (int64_t)(addr - e.last_addr2);

        if (delta == e.corr_delta) {
            e.corr_conf++;
            e.ind_conf++;
        } else {
            e.corr_delta = delta;
            e.corr_conf = 1;
            e.ind_conf = 0;
        }

        e.last_delta = delta;
    }

    e.last_addr2 = addr;
}

// -------------------------------
void custom::update_distance(PTEntry& e) {

    int base = 2;

    // grow distance with confidence (key for timeliness)
    int boost = e.corr_conf / 2 + e.ind_conf;

    e.distance = std::min(MAX_DISTANCE, base + boost);

    // if too many useless prefetches → reduce distance slightly
    if (e.issued > 20 && e.useful < e.issued / 3)
        e.distance = std::max(2, e.distance - 1);
}

// -------------------------------
void custom::adjust_degree(PTEntry& e) {

    if (e.issued < 16)
        return;

    float acc = (float)e.useful / (float)e.issued;

    if (acc < 0.3 && e.degree > MIN_DEGREE)
        e.degree--;
    else if (acc > 0.55 && e.degree < MAX_DEGREE)
        e.degree++;

    e.issued = 0;
    e.useful = 0;
}

// -------------------------------
void custom::issue_prefetch(PTEntry& e) {

    int mode = 0;

    if (e.ind_conf >= IND_CONF)
        mode = 1;
    else if (e.stride_conf >= STRIDE_CONF)
        mode = 2;
    else if (e.corr_conf >= CORR_CONF)
        mode = 3;

    if (mode == 0)
        return;

    for (int i = 1; i <= e.degree; i++) {

        int lookahead = e.distance + i;
        uint64_t pf = 0;

        if (mode == 1) { // indirect-lite (stronger push)
            pf = e.last_addr + e.corr_delta * lookahead * 2;
        }
        else if (mode == 2) {
            pf = e.last_addr + e.stride * lookahead;
        }
        else {
            pf = e.last_addr + e.corr_delta * lookahead;
        }

        // allow limited cross-page (better for indirect)
        if (llabs((int64_t)(pf - e.last_addr)) > (4096 * 4))
            break;

        pf = (pf >> 6) << 6;

        if (intern_->prefetch_line(pf, true, 0))
            e.issued++;
    }
}

// -------------------------------
uint32_t custom::prefetcher_cache_operate(
    champsim::address addr,
    champsim::address ip,
    uint8_t,
    bool useful_prefetch,
    access_type,
    uint32_t metadata_in
) {
    uint64_t a = addr.to<uint64_t>();
    uint64_t pc = ip.to<uint64_t>();

    int idx = find(pc);
    if (idx < 0) idx = alloc(pc);

    PTEntry& e = table[idx];

    update_stride(e, a);
    update_correlation(e, a);
    update_distance(e);

    if (useful_prefetch)
        e.useful++;

    adjust_degree(e);
    issue_prefetch(e);

    return metadata_in;
}

// -------------------------------
uint32_t custom::prefetcher_cache_fill(
    champsim::address,
    long,
    long,
    uint8_t,
    champsim::address,
    uint32_t metadata_in
) {
    return metadata_in;
}