#include "common/dynamic_array.h"
#include "common/string_utils.h"
#include "network/protocol.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace {

using mini_dbms::common::DynamicArray;
using mini_dbms::common::Status;

struct ClientResponse {
    bool ok;
    std::string code;
    std::string message;
    std::size_t affected_rows;
    bool used_index;
    DynamicArray<std::string> columns;
    DynamicArray<DynamicArray<std::string>> rows;

    ClientResponse()
        : ok(false), code(), message(), affected_rows(0), used_index(false), columns(), rows() {}
};

std::string UnescapeField(const std::string& value) {
    std::string out;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '\\' || i + 1 >= value.size()) {
            out += value[i];
            continue;
        }
        ++i;
        if (value[i] == 't') {
            out += '\t';
        } else if (value[i] == 'n') {
            out += '\n';
        } else if (value[i] == 'r') {
            out += '\r';
        } else {
            out += value[i];
        }
    }
    return out;
}

DynamicArray<std::string> SplitEscapedTabs(const std::string& line) {
    DynamicArray<std::string> parts;
    std::size_t start = 0;
    bool escaped = false;
    for (std::size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || (line[i] == '\t' && !escaped)) {
            parts.PushBack(UnescapeField(line.substr(start, i - start)));
            start = i + 1;
            escaped = false;
        } else if (line[i] == '\\' && !escaped) {
            escaped = true;
        } else {
            escaped = false;
        }
    }
    return parts;
}

std::size_t ParseSize(const std::string& text) {
    char* end = nullptr;
    unsigned long long value = std::strtoull(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0') {
        return 0;
    }
    return static_cast<std::size_t>(value);
}

Status ParseResponse(const std::string& body, ClientResponse* response) {
    if (response == nullptr) {
        return Status::InvalidArgument("response output is null");
    }
    DynamicArray<std::string> lines = mini_dbms::common::StringUtils::Split(body, '\n');
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].empty()) {
            continue;
        }
        DynamicArray<std::string> parts = SplitEscapedTabs(lines[i]);
        if (parts.size() == 0) {
            continue;
        }
        if (parts[0] == "STATUS" && parts.size() >= 2) {
            response->ok = parts[1] == "OK";
        } else if (parts[0] == "CODE" && parts.size() >= 2) {
            response->code = parts[1];
        } else if (parts[0] == "MESSAGE" && parts.size() >= 2) {
            response->message = parts[1];
        } else if (parts[0] == "AFFECTED" && parts.size() >= 2) {
            response->affected_rows = ParseSize(parts[1]);
        } else if (parts[0] == "USED_INDEX" && parts.size() >= 2) {
            response->used_index = parts[1] == "1";
        } else if (parts[0] == "COL" && parts.size() >= 2) {
            response->columns.PushBack(parts[1]);
        } else if (parts[0] == "ROW") {
            DynamicArray<std::string> row;
            for (std::size_t j = 1; j < parts.size(); ++j) {
                row.PushBack(parts[j]);
            }
            response->rows.PushBack(std::move(row));
        }
    }
    return Status::OK();
}

std::size_t CellWidth(const std::string& value) {
    return value.size();
}

void PrintSeparator(const DynamicArray<std::size_t>& widths) {
    std::cout << "+";
    for (std::size_t i = 0; i < widths.size(); ++i) {
        for (std::size_t j = 0; j < widths[i] + 2; ++j) {
            std::cout << "-";
        }
        std::cout << "+";
    }
    std::cout << "\n";
}

void PrintPadded(const std::string& value, std::size_t width) {
    std::cout << " " << value;
    for (std::size_t i = CellWidth(value); i < width; ++i) {
        std::cout << " ";
    }
    std::cout << " ";
}

void PrintTable(const ClientResponse& response) {
    DynamicArray<std::size_t> widths;
    for (std::size_t i = 0; i < response.columns.size(); ++i) {
        widths.PushBack(CellWidth(response.columns[i]));
    }
    for (std::size_t row = 0; row < response.rows.size(); ++row) {
        for (std::size_t col = 0; col < response.rows[row].size() && col < widths.size(); ++col) {
            widths[col] = std::max(widths[col], CellWidth(response.rows[row][col]));
        }
    }

    PrintSeparator(widths);
    std::cout << "|";
    for (std::size_t i = 0; i < response.columns.size(); ++i) {
        PrintPadded(response.columns[i], widths[i]);
        std::cout << "|";
    }
    std::cout << "\n";
    PrintSeparator(widths);
    for (std::size_t row = 0; row < response.rows.size(); ++row) {
        std::cout << "|";
        for (std::size_t col = 0; col < response.columns.size(); ++col) {
            const std::string empty;
            const std::string& value = col < response.rows[row].size() ? response.rows[row][col] : empty;
            PrintPadded(value, widths[col]);
            std::cout << "|";
        }
        std::cout << "\n";
    }
    PrintSeparator(widths);
}

void PrintResponse(const ClientResponse& response) {
    if (!response.ok) {
        std::cout << "ERROR " << response.code << ": " << response.message << "\n";
        return;
    }
    if (response.columns.size() > 0) {
        PrintTable(response);
        std::cout << response.rows.size() << " rows in set";
        if (response.used_index) {
            std::cout << " (using index)";
        }
        std::cout << "\n";
        return;
    }
    std::cout << "OK";
    if (!response.message.empty()) {
        std::cout << ": " << response.message;
    }
    if (response.affected_rows > 0) {
        std::cout << " (" << response.affected_rows << " affected)";
    }
    std::cout << "\n";
}

int ParsePort(const char* text) {
    char* end = nullptr;
    long value = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || value <= 0 || value > 65535) {
        return -1;
    }
    return static_cast<int>(value);
}

int ConnectToServer(const char* host, int port) {
    addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string port_text = std::to_string(port);
    addrinfo* results = nullptr;
    int gai_status = getaddrinfo(host, port_text.c_str(), &hints, &results);
    if (gai_status != 0) {
        std::cerr << "getaddrinfo failed: " << gai_strerror(gai_status) << "\n";
        return -1;
    }

    int fd = -1;
    for (addrinfo* item = results; item != nullptr; item = item->ai_next) {
        fd = socket(item->ai_family, item->ai_socktype, item->ai_protocol);
        if (fd < 0) {
            continue;
        }
        if (connect(fd, item->ai_addr, item->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(results);
    return fd;
}

std::string NormalizeInput(const std::string& input) {
    std::string sql = mini_dbms::common::StringUtils::Trim(input);
    if (!sql.empty() && sql[sql.size() - 1] == ';') {
        sql.erase(sql.size() - 1);
        sql = mini_dbms::common::StringUtils::Trim(sql);
    }
    return sql;
}

}  // namespace

int main(int argc, char** argv) {
    const char* host = "127.0.0.1";
    int port = 54321;
    if (argc >= 2) {
        host = argv[1];
    }
    if (argc >= 3) {
        port = ParsePort(argv[2]);
        if (port < 0) {
            std::cerr << "usage: db_client [host] [port]\n";
            return 1;
        }
    }

    int socket_fd = ConnectToServer(host, port);
    if (socket_fd < 0) {
        std::cerr << "failed to connect to " << host << ":" << port << "\n";
        return 1;
    }

    std::string line;
    while (true) {
        std::cout << "mini_dbms> " << std::flush;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }
        std::string sql = NormalizeInput(line);
        if (sql.empty()) {
            continue;
        }
        if (sql == "exit" || sql == "quit") {
            break;
        }

        Status status = mini_dbms::network::SendFrame(socket_fd, sql);
        if (!status.ok()) {
            std::cerr << status.ToString() << "\n";
            break;
        }
        std::string body;
        status = mini_dbms::network::ReceiveFrame(socket_fd, &body);
        if (!status.ok()) {
            std::cerr << status.ToString() << "\n";
            break;
        }
        ClientResponse response;
        status = ParseResponse(body, &response);
        if (!status.ok()) {
            std::cerr << status.ToString() << "\n";
            break;
        }
        PrintResponse(response);
    }

    close(socket_fd);
    return 0;
}
