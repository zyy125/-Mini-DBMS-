#include "testing/test_framework.h"

int main() {
    return mini_dbms::tests::TestRunner::Instance().RunAll();
}
