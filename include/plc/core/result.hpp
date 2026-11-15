#pragma once

#include <string>
#include <variant>
#include <utility>
#include <system_error>

namespace plc::core {

enum class StatusCode {
    Ok = 0,
    InvalidParameter,
    SocketError,
    ConnectionFailed,
    ConnectionTimeout,
    SendFailed,
    ReceiveFailed,
    BufferOverflow,
    ModbusException,
    InvalidPdu,
    TransactionMismatch,
    UnknownError
};

[[nodiscard]] inline std::string status_to_string(StatusCode code) {
    switch (code) {
        case StatusCode::Ok: return "Ok";
        case StatusCode::InvalidParameter: return "Invalid Parameter";
        case StatusCode::SocketError: return "Socket Error";
        case StatusCode::ConnectionFailed: return "Connection Failed";
        case StatusCode::ConnectionTimeout: return "Connection Timeout";
        case StatusCode::SendFailed: return "Send Failed";
        case StatusCode::ReceiveFailed: return "Receive Failed";
        case StatusCode::BufferOverflow: return "Buffer Overflow";
        case StatusCode::ModbusException: return "Modbus Exception";
        case StatusCode::InvalidPdu: return "Invalid PDU";
        case StatusCode::TransactionMismatch: return "Transaction ID Mismatch";
        default: return "Unknown Error";
    }
}

struct Error {
    StatusCode code = StatusCode::Ok;
    std::string message;
    int system_code = 0; // errno or WSAGetLastError

    [[nodiscard]] bool ok() const noexcept { return code == StatusCode::Ok; }
    [[nodiscard]] std::string to_string() const {
        if (message.empty()) {
            return status_to_string(code);
        }
        return status_to_string(code) + ": " + message;
    }
};

template <typename T>
class Result {
public:
    Result(T val) : storage_(std::move(val)) {}
    Result(Error err) : storage_(std::move(err)) {}
    Result(StatusCode code, std::string msg = "") : storage_(Error{code, std::move(msg), 0}) {}

    [[nodiscard]] bool ok() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return ok();
    }

    [[nodiscard]] const T& value() const & {
        return std::get<T>(storage_);
    }

    [[nodiscard]] T& value() & {
        return std::get<T>(storage_);
    }

    [[nodiscard]] T&& value() && {
        return std::get<T>(std::move(storage_));
    }

    [[nodiscard]] const Error& error() const {
        return std::get<Error>(storage_);
    }

private:
    std::variant<T, Error> storage_;
};

// Specialization for void
template <>
class Result<void> {
public:
    Result() : error_{StatusCode::Ok, "", 0} {}
    Result(Error err) : error_(std::move(err)) {}
    Result(StatusCode code, std::string msg = "") : error_{code, std::move(msg), 0} {}

    [[nodiscard]] bool ok() const noexcept {
        return error_.code == StatusCode::Ok;
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return ok();
    }

    [[nodiscard]] const Error& error() const {
        return error_;
    }

private:
    Error error_;
};

} // namespace plc::core
