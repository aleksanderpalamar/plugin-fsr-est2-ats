#include "temporal/jitter_tracker.hpp"

using namespace neuralfx::temporal;

namespace {
bool same(std::optional<Jitter> actual, Jitter expected) {
    return actual && actual->x == expected.x && actual->y == expected.y;
}

int unsynced_case() {
    JitterTracker tracker;
    return tracker.predict(0) ? 10 : 0;
}

int sync_case() {
    JitterTracker tracker;
    if (tracker.observe({100, sequence_jitter(5)}) != SyncResult::Resynced) return 20;
    if (!same(tracker.predict(103), sequence_jitter(8))) return 21;
    if (!same(tracker.predict(100 + jitter_period * 3 + 1), sequence_jitter(6))) return 22;
    if (!same(tracker.predict(99), sequence_jitter(4))) return 23;
    if (tracker.observe({102, sequence_jitter(7)}) != SyncResult::Confirmed) return 24;
    return 0;
}

int mismatch_case() {
    JitterTracker tracker;
    tracker.observe({10, sequence_jitter(0)});
    if (tracker.observe({12, sequence_jitter(9)}) != SyncResult::Resynced) return 30;
    if (!same(tracker.predict(13), sequence_jitter(10))) return 31;
    return 0;
}

int lost_case() {
    JitterTracker tracker;
    tracker.observe({10, sequence_jitter(0)});
    if (tracker.observe({11, Jitter{0.0f, 0.0f}}) != SyncResult::Lost) return 40;
    if (tracker.predict(12)) return 41;
    tracker.observe({20, sequence_jitter(2)});
    tracker.reset();
    return tracker.predict(21) ? 42 : 0;
}
}

int main() {
    for (auto test_case : {unsynced_case, sync_case, mismatch_case, lost_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
