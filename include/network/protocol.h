#pragma once

#include <cstdint>
#include <string>

#include "common/status.h"

namespace mini_dbms::network {

constexpr std::uint32_t kMaxPayloadBytes = 16U * 1024U * 1024U;

common::Status SendFrame(int socket_fd, const std::string& payload);
common::Status ReceiveFrame(int socket_fd, std::string* payload);

}  // namespace mini_dbms::network
