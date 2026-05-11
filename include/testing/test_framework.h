#pragma once

#include <iostream>
#include <sstream>
#include <string>

#include "common/status.h"

namespace mini_dbms::tests {

class TestRunner {
public:
    using TestFunc = void (*)();

    struct TestCase {
        const char* name;
        TestFunc func;
        TestCase* next;
    };

    static TestRunner& Instance() {
        static TestRunner runner;
        return runner;
    }

    void Register(TestCase* test_case) {
        test_case->next = head_;
        head_ = test_case;
    }

    int RunAll() {
        int total = 0;
        int failed = 0;
        for (TestCase* test_case = head_; test_case != nullptr; test_case = test_case->next) {
            ++total;
            current_failed_ = false;
            std::cout << "[ RUN      ] " << test_case->name << '\n';
            test_case->func();
            if (current_failed_) {
                ++failed;
                std::cout << "[  FAILED  ] " << test_case->name << '\n';
            } else {
                std::cout << "[       OK ] " << test_case->name << '\n';
            }
        }
        std::cout << "[==========] " << total << " tests ran\n";
        std::cout << "[  PASSED  ] " << (total - failed) << " tests\n";
        if (failed != 0) {
            std::cout << "[  FAILED  ] " << failed << " tests\n";
        }
        return failed == 0 ? 0 : 1;
    }

    void Fail(const char* file, int line, const std::string& message) {
        current_failed_ = true;
        std::cout << file << ':' << line << ": Failure\n" << message << '\n';
    }

private:
    TestRunner() : head_(nullptr), current_failed_(false) {}

    TestCase* head_;
    bool current_failed_;
};

class TestRegistrar {
public:
    TestRegistrar(TestRunner::TestCase* test_case) {
        TestRunner::Instance().Register(test_case);
    }
};

template <typename Expected, typename Actual>
void ExpectEq(const Expected& expected,
              const Actual& actual,
              const char* expected_expr,
              const char* actual_expr,
              const char* file,
              int line) {
    if (!(expected == actual)) {
        std::ostringstream message;
        message << "Expected equality of these values:\n  " << expected_expr << "\n  "
                << actual_expr << "\nActual values:\n  " << expected << "\n  " << actual;
        TestRunner::Instance().Fail(file, line, message.str());
    }
}

inline void ExpectTrue(bool value, const char* expr, const char* file, int line) {
    if (!value) {
        std::ostringstream message;
        message << "Expected true: " << expr;
        TestRunner::Instance().Fail(file, line, message.str());
    }
}

inline void ExpectStatusOk(const mini_dbms::common::Status& status,
                           const char* expr,
                           const char* file,
                           int line) {
    if (!status.ok()) {
        std::ostringstream message;
        message << "Expected OK status: " << expr << "\nActual: " << status.ToString();
        TestRunner::Instance().Fail(file, line, message.str());
    }
}

}  // namespace mini_dbms::tests

#define MINI_DBMS_TEST_CONCAT_IMPL(a, b) a##b
#define MINI_DBMS_TEST_CONCAT(a, b) MINI_DBMS_TEST_CONCAT_IMPL(a, b)

#define TEST_CASE(name)                                                        \
    static void MINI_DBMS_TEST_CONCAT(TestFunc_, __LINE__)();                  \
    static ::mini_dbms::tests::TestRunner::TestCase                            \
        MINI_DBMS_TEST_CONCAT(TestCase_, __LINE__) = {                         \
            name, &MINI_DBMS_TEST_CONCAT(TestFunc_, __LINE__), nullptr};       \
    static ::mini_dbms::tests::TestRegistrar                                   \
        MINI_DBMS_TEST_CONCAT(TestRegistrar_, __LINE__)(                       \
            &MINI_DBMS_TEST_CONCAT(TestCase_, __LINE__));                      \
    static void MINI_DBMS_TEST_CONCAT(TestFunc_, __LINE__)()

#define EXPECT_TRUE(expr) \
    ::mini_dbms::tests::ExpectTrue(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

#define EXPECT_EQ(expected, actual) \
    ::mini_dbms::tests::ExpectEq((expected), (actual), #expected, #actual, __FILE__, __LINE__)

#define EXPECT_STATUS_OK(status_expr) \
    ::mini_dbms::tests::ExpectStatusOk((status_expr), #status_expr, __FILE__, __LINE__)
