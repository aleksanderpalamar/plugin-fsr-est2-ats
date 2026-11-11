#include "upscale/upscale_counters.hpp"

#include <iostream>

using namespace neuralfx::upscale;

int main() {
    UpscaleCounters counters;
    counters.outputs = 600;
    counters.replaced = 590;
    counters.upscaled = 480;
    counters.contract_invalid = 70;
    counters.backend_invalid = 30;
    counters.failed = 10;
    counters.not_replaced = 10;
    auto line = counters_line(counters);
    std::string wanted = "Upscale summary outputs=600 replaced=590 upscaled=480 contract_invalid=70 backend_invalid=30 "
        "failed=10 not_replaced=10";
    if (line != wanted) {
        std::cerr << line << '\n';
        return 1;
    }
    if (!summary_due(600) || summary_due(0) || summary_due(599) || !summary_due(1200)) return 2;
    return counters_line({}) == "Upscale summary outputs=0 replaced=0 upscaled=0 contract_invalid=0 backend_invalid=0 "
        "failed=0 not_replaced=0" ? 0 : 3;
}
