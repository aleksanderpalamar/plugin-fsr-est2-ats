#include "config/config.hpp"
#include "common/logger.hpp"

#include <fstream>

namespace neuralfx {
Config load_config() {
    std::ifstream file(module_directory() / L"NeuralFX" / L"neuralfx.ini");
    if (!file) return {};
    return parse_config(file);
}
}
