#include "common/status.h"

#include "testing/test_framework.h"

using mini_dbms::common::Status;
using mini_dbms::common::StatusCode;

TEST_CASE("Status represents success and failure") {
    Status ok = Status::OK();
    EXPECT_TRUE(ok.ok());
    EXPECT_EQ(std::string("OK"), ok.ToString());

    Status error = Status::InvalidArgument("bad name");
    EXPECT_TRUE(!error.ok());
    EXPECT_EQ(StatusCode::kInvalidArgument, error.code());
    EXPECT_EQ(std::string("bad name"), error.message());
    EXPECT_EQ(std::string("InvalidArgument: bad name"), error.ToString());
}
