#pragma once
#include <cstdio>
#include <cstring>
#include <string>

// A minimal test harness: each failed check prints and counts; main returns
// the failure count.
extern int g_failures;
extern int g_checks;

#define CHECK(cond)                                                                                  \
    do {                                                                                             \
        g_checks++;                                                                                  \
        if (!(cond)) {                                                                               \
            g_failures++;                                                                            \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond);                \
        }                                                                                            \
    } while (0)

#define CHECK_STR(actual, expected)                                                                  \
    do {                                                                                             \
        g_checks++;                                                                                  \
        std::string a_ = (actual), e_ = (expected);                                                  \
        if (a_ != e_) {                                                                              \
            g_failures++;                                                                            \
            fprintf(stderr, "%s:%d: expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, e_.c_str(), \
                    a_.c_str());                                                                     \
        }                                                                                            \
    } while (0)

void test_qbit_chain();
void test_qbit_address();
void test_explorer_parse();
