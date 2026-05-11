#pragma once

#include <ostream>
#include <string>

namespace mini_dbms::common {

enum class StatusCode {
    kOk = 0,
    kInvalidArgument,
    kNotFound,
    kAlreadyExists,
    kIOError,
    kOutOfMemory,
    kInternal
};

class Status {
public:
    Status() : code_(StatusCode::kOk), message_() {}
    Status(StatusCode code, std::string message)
        : code_(code), message_(message) {}

    static Status OK() { return Status(); }
    static Status InvalidArgument(const std::string& message) {
        return Status(StatusCode::kInvalidArgument, message);
    }
    static Status NotFound(const std::string& message) {
        return Status(StatusCode::kNotFound, message);
    }
    static Status AlreadyExists(const std::string& message) {
        return Status(StatusCode::kAlreadyExists, message);
    }
    static Status IOError(const std::string& message) {
        return Status(StatusCode::kIOError, message);
    }
    static Status OutOfMemory(const std::string& message) {
        return Status(StatusCode::kOutOfMemory, message);
    }
    static Status Internal(const std::string& message) {
        return Status(StatusCode::kInternal, message);
    }

    bool ok() const { return code_ == StatusCode::kOk; }
    StatusCode code() const { return code_; }
    const std::string& message() const { return message_; }

    std::string ToString() const {
        if (ok()) {
            return "OK";
        }
        return CodeName(code_) + ": " + message_;
    }

    static std::string CodeName(StatusCode code) {
        switch (code) {
            case StatusCode::kOk:
                return "OK";
            case StatusCode::kInvalidArgument:
                return "InvalidArgument";
            case StatusCode::kNotFound:
                return "NotFound";
            case StatusCode::kAlreadyExists:
                return "AlreadyExists";
            case StatusCode::kIOError:
                return "IOError";
            case StatusCode::kOutOfMemory:
                return "OutOfMemory";
            case StatusCode::kInternal:
                return "Internal";
        }
        return "Unknown";
    }

private:
    StatusCode code_;
    std::string message_;
};

inline std::ostream& operator<<(std::ostream& os, StatusCode code) {
    os << Status::CodeName(code);
    return os;
}

}  // namespace mini_dbms::common
