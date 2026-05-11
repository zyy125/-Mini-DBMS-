#include "network/protocol.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace mini_dbms::network {
namespace {

common::Status SendAll(int socket_fd, const char* data, std::size_t size) {
    std::size_t sent = 0;
    while (sent < size) {
        ssize_t written = send(socket_fd, data + sent, size - sent, 0);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return common::Status::IOError(std::string("send failed: ") + std::strerror(errno));
        }
        if (written == 0) {
            return common::Status::IOError("send closed unexpectedly");
        }
        sent += static_cast<std::size_t>(written);
    }
    return common::Status::OK();
}

common::Status ReceiveAll(int socket_fd, char* data, std::size_t size) {
    std::size_t received = 0;
    while (received < size) {
        ssize_t n = recv(socket_fd, data + received, size - received, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return common::Status::IOError(std::string("recv failed: ") + std::strerror(errno));
        }
        if (n == 0) {
            return common::Status::IOError("peer closed connection");
        }
        received += static_cast<std::size_t>(n);
    }
    return common::Status::OK();
}

}  // namespace

common::Status SendFrame(int socket_fd, const std::string& payload) {
    if (payload.size() > kMaxPayloadBytes) {
        return common::Status::InvalidArgument("payload exceeds protocol limit");
    }
    std::uint32_t length = htonl(static_cast<std::uint32_t>(payload.size()));
    common::Status status =
        SendAll(socket_fd, reinterpret_cast<const char*>(&length), sizeof(length));
    if (!status.ok()) {
        return status;
    }
    if (payload.empty()) {
        return common::Status::OK();
    }
    return SendAll(socket_fd, payload.data(), payload.size());
}

common::Status ReceiveFrame(int socket_fd, std::string* payload) {
    if (payload == nullptr) {
        return common::Status::InvalidArgument("payload output is null");
    }
    std::uint32_t network_length = 0;
    common::Status status =
        ReceiveAll(socket_fd, reinterpret_cast<char*>(&network_length), sizeof(network_length));
    if (!status.ok()) {
        return status;
    }
    std::uint32_t length = ntohl(network_length);
    if (length > kMaxPayloadBytes) {
        return common::Status::InvalidArgument("payload exceeds protocol limit");
    }
    payload->assign(length, '\0');
    if (length == 0) {
        return common::Status::OK();
    }
    return ReceiveAll(socket_fd, &(*payload)[0], length);
}

}  // namespace mini_dbms::network
