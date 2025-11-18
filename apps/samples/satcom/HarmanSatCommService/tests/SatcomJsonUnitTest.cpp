#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include "../ConfigLoader/NtnConfigLoader.hpp"

namespace fs = std::filesystem;
using ntncfg::Loader;

struct Result { std::string path; bool parsed{false}; bool ok{false}; };

int main(int argc, char** argv) {
    fs::path cfg_dir = (argc > 1) ? fs::path(argv[1]) : fs::path("../Config");
    if (!fs::exists(cfg_dir)) {
        std::cerr << "Config dir not found: " << cfg_dir << "\n";
        return 2;
    }

    std::vector<Result> rows;
    for (auto& p : fs::directory_iterator(cfg_dir)) {
        if (!p.is_regular_file() || p.path().extension() != ".json") continue;

        Loader ld;
        ld.load(p.path().string());

        std::cout << "\n===== FILE: " << p.path().string() << " =====\n";
        ld.dump(std::cout);

        Result r;
        r.path   = p.path().string();
        r.parsed = ld.parsed_ok();
        r.ok     = ld.ok();
        std::cout << "[UNITTEST] PARSE: " << (r.parsed ? "PASS" : "FAIL")
                  << "   VALIDATION: " << (r.ok ? "PASS" : "WARN/ERR present") << "\n";
        rows.push_back(r);
    }

    size_t pass = 0;
    for (auto& r : rows) pass += (r.parsed && r.ok) ? 1 : 0;
    std::cout << "\nSUMMARY: " << pass << "/" << rows.size() << " files parsed & validated cleanly.\n";
    return (pass == rows.size()) ? 0 : 1;
}
