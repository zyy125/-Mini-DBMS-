#include "executor/executor.h"
#include "network/protocol.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

using mini_dbms::common::Status;
using mini_dbms::executor::Executor;
using mini_dbms::executor::QueryResult;
using mini_dbms::executor::QueryValue;
using mini_dbms::storage::ColumnType;

std::string EscapeField(const std::string& value) {
    std::string escaped;
    for (std::size_t i = 0; i < value.size(); ++i) {
        char ch = value[i];
        if (ch == '\\') {
            escaped += "\\\\";
        } else if (ch == '\t') {
            escaped += "\\t";
        } else if (ch == '\n') {
            escaped += "\\n";
        } else if (ch == '\r') {
            escaped += "\\r";
        } else {
            escaped += ch;
        }
    }
    return escaped;
}

std::string ValueToString(const QueryValue& value) {
    if (value.type == ColumnType::kInt) {
        return std::to_string(value.int_value);
    }
    return value.string_value;
}

std::string SerializeResult(const QueryResult& result) {
    std::string body;
    body += "STATUS\t";
    body += result.status.ok() ? "OK\n" : "ERROR\n";
    body += "CODE\t";
    body += EscapeField(Status::CodeName(result.status.code()));
    body += "\n";
    body += "MESSAGE\t";
    body += EscapeField(result.status.ok() ? result.message : result.status.message());
    body += "\n";
    body += "AFFECTED\t";
    body += std::to_string(result.affected_rows);
    body += "\n";
    body += "USED_INDEX\t";
    body += result.used_index ? "1\n" : "0\n";
    body += "COLUMNS\t";
    body += std::to_string(result.columns.size());
    body += "\n";
    for (std::size_t i = 0; i < result.columns.size(); ++i) {
        body += "COL\t";
        body += EscapeField(result.columns[i]);
        body += "\n";
    }
    body += "ROWS\t";
    body += std::to_string(result.rows.size());
    body += "\n";
    for (std::size_t row_index = 0; row_index < result.rows.size(); ++row_index) {
        body += "ROW";
        for (std::size_t value_index = 0; value_index < result.rows[row_index].values.size();
             ++value_index) {
            body += "\t";
            body += EscapeField(ValueToString(result.rows[row_index].values[value_index]));
        }
        body += "\n";
    }
    return body;
}

int ParsePort(const char* text) {
    char* end = nullptr;
    long value = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || value <= 0 || value > 65535) {
        return -1;
    }
    return static_cast<int>(value);
}

Status HandleClient(int client_fd, Executor* executor) {
    while (true) {
        std::string sql;
        Status status = mini_dbms::network::ReceiveFrame(client_fd, &sql);
        if (!status.ok()) {
            return status;
        }
        QueryResult result = executor->ExecuteSql(sql);
        status = mini_dbms::network::SendFrame(client_fd, SerializeResult(result));
        if (!status.ok()) {
            return status;
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::signal(SIGPIPE, SIG_IGN);

    int port = 54321;
    if (argc >= 2) {
        port = ParsePort(argv[1]);
        if (port < 0) {
            std::cerr << "usage: db_server [port] [data_root]\n";
            return 1;
        }
    }
    std::filesystem::path data_root = "data";
    if (argc >= 3) {
        data_root = argv[2];
    }

    Executor executor(data_root);
    Status status = executor.Initialize();
    if (!status.ok()) {
        std::cerr << "failed to initialize executor: " << status.ToString() << "\n";
        return 1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "socket failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "bind failed: " << std::strerror(errno) << "\n";
        close(server_fd);
        return 1;
    }
    if (listen(server_fd, 16) < 0) {
        std::cerr << "listen failed: " << std::strerror(errno) << "\n";
        close(server_fd);
        return 1;
    }

    std::cout << "db_server listening on 0.0.0.0:" << port << " data_root=" << data_root << "\n";
    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "accept failed: " << std::strerror(errno) << "\n";
            continue;
        }
        Status client_status = HandleClient(client_fd, &executor);
        if (!client_status.ok() && client_status.message() != "peer closed connection") {
            std::cerr << "client disconnected: " << client_status.ToString() << "\n";
        }
        close(client_fd);
    }
}
