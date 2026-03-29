#pragma once
#include <stdexcept>
#include <string>
#include <chrono>

enum class SocketStateError{
	None,
	NullSocket,
	NotOpen,
	NotConnected,
	RemoteClosed,
	NetworkError
};

enum class SendError{ None, NotConnected, Timeout, NetworkError, BufferOverflow };
enum class ReceiveError{ None, Timeout, ConnectionClosed, NetworkError, BufferError };
enum class AcceptError{ None, Transient, Resource, Fatal, Timeout };

class CommunicatorException : public std::exception{
protected:
    SocketStateError code;
    std::string context;
    std::string message;

public:
    CommunicatorException(SocketStateError c, const std::string& ctx, const std::string& msg)
        : code(c), context(ctx), message(ctx + ": " + msg) {}

    SocketStateError getCode() const noexcept{ return code; }
    const char* what() const noexcept override{ return message.c_str(); }
    const std::string& getContext() const noexcept{ return context; }
};

class ConnectionException : public CommunicatorException{
public:
    ConnectionException(const std::string& ctx, const std::string& msg)
        : CommunicatorException(SocketStateError::NotConnected, ctx, msg) {}
    
    ConnectionException(SocketStateError c, const std::string& ctx, const std::string& msg)
        : CommunicatorException(c, ctx, msg) {}
};

class SendException : public CommunicatorException{
private:
    SendError sendError;

public:
    SendException(SendError err, const std::string& ctx, const std::string& msg)
        : CommunicatorException(SocketStateError::NetworkError, ctx, msg), sendError(err) {}

    SendError getSendError() const noexcept{ return sendError; }
};

class ReceiveException : public CommunicatorException{
private:
    ReceiveError recvError;

public:
    ReceiveException(ReceiveError err, const std::string& ctx, const std::string& msg)
        : CommunicatorException(SocketStateError::NetworkError, ctx, msg), recvError(err) {}

    ReceiveError getReceiveError() const noexcept{ return recvError; }
};

class AcceptException : public CommunicatorException{
private:
    AcceptError acceptError;

public:
    AcceptException(AcceptError err, const std::string& ctx, const std::string& msg)
        : CommunicatorException(SocketStateError::NetworkError, ctx, msg), acceptError(err) {}

    AcceptError getAcceptError() const noexcept{ return acceptError; }
};

class OperationTimeoutException : public CommunicatorException{
private:
    std::chrono::milliseconds elapsed;

public:
    OperationTimeoutException(const std::string& ctx, std::chrono::milliseconds elapsedMs, const std::string& msg)
        : CommunicatorException(SocketStateError::NetworkError, ctx, msg), elapsed(elapsedMs) {}

    std::chrono::milliseconds getElapsed() const noexcept{ return elapsed; }
};

struct OperationConfig{
    std::chrono::milliseconds connectTimeout{5000};
    std::chrono::milliseconds receiveTimeout{3000};
    std::chrono::milliseconds sendTimeout{3000};
    std::chrono::milliseconds heartbeatInterval{30000};
    int maxRetries{3};
    bool enableHeartbeat{true};
    bool enableAutoReconnect{true};
};

enum class ConnectionHealth{ Healthy, Degraded, Unhealthy, Unknown };

struct ConnectionStatus{
    ConnectionHealth health{ConnectionHealth::Unknown};
    std::chrono::steady_clock::time_point lastHeartbeat;
    std::chrono::steady_clock::time_point lastActivity;
    int consecutiveFailures{0};
    int maxFailuresBeforeDisconnect{5};
};
