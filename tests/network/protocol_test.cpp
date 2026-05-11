#include "network/protocol.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <string>

#include "common/status.h"
#include "testing/test_framework.h"

using mini_dbms::common::StatusCode;

namespace {

class SocketPair {
public:
    SocketPair() : fds_{-1, -1} {
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds_) != 0) {
            fds_[0] = -1;
            fds_[1] = -1;
        }
    }

    ~SocketPair() {
        if (fds_[0] >= 0) {
            close(fds_[0]);
        }
        if (fds_[1] >= 0) {
            close(fds_[1]);
        }
    }

    int first() const { return fds_[0]; }
    int second() const { return fds_[1]; }
    bool valid() const { return fds_[0] >= 0 && fds_[1] >= 0; }

private:
    int fds_[2];
};

}  // namespace

TEST_CASE("Protocol sends and receives framed payload") {
    SocketPair sockets;
    EXPECT_TRUE(sockets.valid());

    const std::string payload = "select * from students where id = 1";
    EXPECT_STATUS_OK(mini_dbms::network::SendFrame(sockets.first(), payload));

    std::string received;
    EXPECT_STATUS_OK(mini_dbms::network::ReceiveFrame(sockets.second(), &received));
    EXPECT_EQ(payload, received);
}

TEST_CASE("Protocol handles empty payload and null output") {
    SocketPair sockets;
    EXPECT_TRUE(sockets.valid());

    EXPECT_STATUS_OK(mini_dbms::network::SendFrame(sockets.first(), ""));
    std::string received = "not empty";
    EXPECT_STATUS_OK(mini_dbms::network::ReceiveFrame(sockets.second(), &received));
    EXPECT_EQ(std::string(""), received);

    EXPECT_EQ(StatusCode::kInvalidArgument,
              mini_dbms::network::ReceiveFrame(sockets.second(), nullptr).code());
}

TEST_CASE("Protocol rejects oversized incoming payload before allocation") {
    SocketPair sockets;
    EXPECT_TRUE(sockets.valid());

    std::uint32_t length = htonl(mini_dbms::network::kMaxPayloadBytes + 1U);
    ssize_t written = send(sockets.first(), &length, sizeof(length), 0);
    EXPECT_EQ(static_cast<ssize_t>(sizeof(length)), written);

    std::string received;
    EXPECT_EQ(StatusCode::kInvalidArgument,
              mini_dbms::network::ReceiveFrame(sockets.second(), &received).code());
}
