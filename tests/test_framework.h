#pragma once

// Tiny zero-dependency test harness. Not a core module -- infrastructure only.

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace mirtest {

struct Case {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

inline int& failures() {
    static int n = 0;
    return n;
}

inline int& skips() {
    static int n = 0;
    return n;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

inline int run() {
    int passed = 0;
    for (const Case& c : registry()) {
        const int before = failures();
        try {
            c.fn();
        } catch (const std::exception& e) {
            std::cerr << "  [ERROR] " << c.name << ": " << e.what() << "\n";
            ++failures();
        }
        if (failures() == before) {
            std::cout << "  [PASS]  " << c.name << "\n";
            ++passed;
        }
    }
    std::cout << "\n" << passed << " passed, " << failures() << " failed, "
              << skips() << " skipped\n";
    return failures() == 0 ? 0 : 1;
}

}  // namespace mirtest

#define TEST(name)                                                         \
    static void name();                                                    \
    static ::mirtest::Registrar reg_##name(#name, name);                   \
    static void name()

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << "  [FAIL]  " << __FILE__ << ":" << __LINE__        \
                      << " CHECK(" #cond ")\n";                            \
            ++::mirtest::failures();                                        \
        }                                                                  \
    } while (0)

#define CHECK_EQ(a, b)                                                     \
    do {                                                                   \
        if (!((a) == (b))) {                                               \
            std::cerr << "  [FAIL]  " << __FILE__ << ":" << __LINE__        \
                      << " CHECK_EQ(" #a ", " #b ") -> " << (a) << " != "   \
                      << (b) << "\n";                                       \
            ++::mirtest::failures();                                        \
        }                                                                  \
    } while (0)

// Use for functionality that depends on a Day-N TODO you haven't done yet.
#define SKIP(reason)                                                       \
    do {                                                                   \
        std::cout << "  [SKIP]  " << reason << "\n";                       \
        ++::mirtest::skips();                                              \
        return;                                                            \
    } while (0)
