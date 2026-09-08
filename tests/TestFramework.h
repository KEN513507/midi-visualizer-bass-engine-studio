#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

// Minimal dependency-free test harness. No network fetch is required to
// build tests on an offline machine.
namespace testfw {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> cases;
    return cases;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back(TestCase{name, std::move(fn)});
    }
};

struct AssertionFailure {
    std::string message;
};

inline int runAll() {
    int failed = 0;
    for (const auto& test : registry()) {
        try {
            test.fn();
            std::printf("[PASS] %s\n", test.name.c_str());
        } catch (const AssertionFailure& failure) {
            std::printf("[FAIL] %s: %s\n", test.name.c_str(), failure.message.c_str());
            ++failed;
        } catch (const std::exception& ex) {
            std::printf("[FAIL] %s: unexpected exception: %s\n", test.name.c_str(), ex.what());
            ++failed;
        }
    }
    std::printf("---\n%zu tests, %d failed\n", registry().size(), failed);
    return failed;
}

}  // namespace testfw

#define TEST_CASE(name)                                                                \
    static void name();                                                               \
    static ::testfw::Registrar registrar_##name(#name, name);                          \
    static void name()

#define REQUIRE(cond)                                                                  \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            throw ::testfw::AssertionFailure{std::string("REQUIRE failed: ") + #cond}; \
        }                                                                              \
    } while (0)

#define REQUIRE_EQ(a, b)                                                               \
    do {                                                                               \
        if (!((a) == (b))) {                                                           \
            throw ::testfw::AssertionFailure{std::string("REQUIRE_EQ failed: ") + #a + \
                                              " != " + #b};                            \
        }                                                                              \
    } while (0)

#define REQUIRE_FALSE(cond) REQUIRE(!(cond))
