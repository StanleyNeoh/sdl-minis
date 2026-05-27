#ifndef PLATFORM_SOCKET_HPP
#define PLATFORM_SOCKET_HPP

#ifdef _WIN32
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <iostream>

	using socklen_t = int;
	using ssize_t = int;
	using in_port_t = unsigned short;
	using in_addr_t = unsigned long;

	struct SocketSession {
		bool initialized = false;

		SocketSession() {
			WSADATA data{};
			initialized = WSAStartup(MAKEWORD(2, 2), &data) == 0;
			if (!initialized) {
				std::cerr << "WSAStartup failed: " << WSAGetLastError() << "\n";
			}
		}

		~SocketSession() {
			if (initialized) {
				WSACleanup();
			}
		}

		SocketSession(const SocketSession&) = delete;
		SocketSession& operator=(const SocketSession&) = delete;
	};

	inline int close(int socket) {
		return closesocket(socket);
	}

	inline int socket_error() {
		return WSAGetLastError();
	}
#else
	#include <arpa/inet.h>
	#include <cerrno>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <unistd.h>
	#include <fcntl.h>

	struct SocketSession {
		bool initialized = true;
	};

	inline int socket_error() {
		return errno;
	}
#endif

#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <iostream>

sockaddr_in create_sockaddr(in_addr_t addr, in_port_t port, bool network_endian=false) {
    sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET;
	if (!network_endian) {
		port = htons(port);
	}
    socketAddress.sin_port = port;
    socketAddress.sin_addr.s_addr = addr;
	return socketAddress;
}

inline const sockaddr* sockaddr_cast(const sockaddr_in* address) {
    return reinterpret_cast<const sockaddr*>(address);
}

inline sockaddr* sockaddr_cast(sockaddr_in* address) {
    return reinterpret_cast<sockaddr*>(address);
}

struct SocketResource {
	int _socket = -1;

	SocketResource() = default;
	SocketResource(int fd): _socket(fd) {}
	SocketResource(int domain, int type, int protocol): _socket(socket(domain, type, protocol)) {}
	SocketResource(const SocketResource&) = delete;
	SocketResource& operator=(const SocketResource& other) = delete;
	SocketResource(SocketResource&& other): _socket(other._socket) {
		other._socket = -1;
	}
	SocketResource& operator=(SocketResource&& other) noexcept {
		this->~SocketResource();
		_socket = other._socket;
		other._socket = -1;
		return *this;
	}

	~SocketResource() {
		if (is_available()) {
			close(_socket);
			_socket = -1;
		}
	}

	bool is_available() const {
		return _socket >= 0;
	}

	operator int() const {
		return _socket;
	}

	template <bool opt>
	bool set_blocking() {
	#ifdef _WIN32
		u_long iMode;
		if constexpr (opt) {
			iMode = 0;
		} else {
			iMode = 1;
		}
		int iResult = ioctlsocket(_socket, FIONBIO, &iMode);
		return iResult == NO_ERROR;
	#else
		int flags = fcntl(_socket, F_GETFL, 0);
		if (flags == -1) return false;
		if constexpr (opt) {
			flags &= ~O_NONBLOCK;
		} else {
			flags |= O_NONBLOCK;
		}
		int result = fcntl(_socket, F_SETFL, flags);
		return result != -1;
	#endif
	}

	template <typename T>
	int setsockopt(int opt_name, const T& opt_val) const {
		if (!is_available()) return -1;
		return ::setsockopt(_socket, SOL_SOCKET, opt_name, &opt_val, sizeof(opt_val));
	}

	int connect(const sockaddr_in& address) const {
		if (!is_available()) return -1;
		return ::connect(_socket, sockaddr_cast(&address), sizeof(address));
	}

	int bind(const sockaddr_in& address) const {
		if (!is_available()) return -1;
		return ::bind(_socket, sockaddr_cast(&address), sizeof(address));
	}

	int listen(int nconn) const {
		if (!is_available()) return -1;
		return ::listen(_socket, nconn);
	}

	int getsockname(sockaddr_in& address) const {
		socklen_t socklen = sizeof(address);
		return ::getsockname(_socket, sockaddr_cast(&address), &socklen);
	}

	int sendto(const void* buf, size_t buf_size, const sockaddr_in& address, int flags = 0) const {
		return ::sendto(_socket, buf, buf_size, flags, sockaddr_cast(&address), sizeof(address));
	}

	int recvfrom(void* buf, size_t buf_size, sockaddr_in& address, int flags = 0) const {
		socklen_t socklen = sizeof(address);
		return ::recvfrom(_socket, buf, buf_size, flags, sockaddr_cast(&address), &socklen);
	}

	int send(const void* buf, size_t buf_size, int flags = 0) const {
		return ::send(_socket, buf, buf_size, flags);
	}

	int recv(void* buf, size_t buf_size, int flags = 0) const {
		return ::recv(_socket, buf, buf_size, flags);
	}

	SocketResource accept(sockaddr_in& address) const {
		socklen_t socklen = sizeof(address);
		SocketResource client(::accept(_socket, sockaddr_cast(&address), &socklen));
		return client;
	}

	bool recv_exact(void* buffer, std::size_t size) const {
		auto* bytes = static_cast<char*>(buffer);
		std::size_t received = 0;
		while (received < size) {
			ssize_t nbytes = recv(bytes + received, size - received, 0);
			if (nbytes <= 0) {
				return false;
			}
			received += static_cast<std::size_t>(nbytes);
		}
		return true;
	}

	bool send_exact(const void* buffer, std::size_t size) const {
		const auto* bytes = static_cast<const char*>(buffer);
		std::size_t sent = 0;
		while (sent < size) {
			ssize_t nbytes = send(bytes + sent, size - sent, 0);
			if (nbytes <= 0) {
				return false;
			}
			sent += static_cast<std::size_t>(nbytes);
		}
		return true;
	}

	bool recv_i32(int& out) const {
		std::int32_t value = 0;
		if (!recv_exact(&value, sizeof(value))) {
			return false;
		}
		out = ntohl(value);
		return true;
	}

	static void append_i32(std::string& out, int value) {
		std::int32_t encoded = htonl(static_cast<std::int32_t>(value));
		std::size_t old_size = out.size();
		out.resize(old_size + sizeof(encoded));
		std::memcpy(out.data() + old_size, &encoded, sizeof(encoded));
	}


	bool send_i32(int value) const {
		std::int32_t encoded = htonl(static_cast<std::int32_t>(value));
		return send_exact(&encoded, sizeof(encoded));
	}

	bool send_string(const std::string& out) const {
		return send_i32(static_cast<int>(out.size()))
			&& send_exact(out.data(), out.size());
	}

	bool recv_string(std::string& out) const {
		int ssize = 0;
		if (!recv_i32(ssize)) return false;
		out.resize(ssize);
		return ssize == 0 || recv_exact(out.data(), static_cast<std::size_t>(ssize));
	}
};

in_addr_t own_ip_address() {
	SocketResource socketResource(AF_INET, SOCK_DGRAM, 0);
	sockaddr_in addr = create_sockaddr(INADDR_LOOPBACK, 123);
	socketResource.connect(addr);

	sockaddr_in local;
	if (socketResource.getsockname(local)) {
		return INADDR_ANY;
	}
	return local.sin_addr.s_addr;
}

std::ostream& operator<<(std::ostream& o, const sockaddr_in& addr) {
	char address[INET_ADDRSTRLEN] = {0};
	inet_ntop(AF_INET, &addr.sin_addr.s_addr, address, sizeof(address));
	o << address << ":" << ntohs(addr.sin_port);
	return o;
}

inline bool operator==(const sockaddr_in& addr1, const sockaddr_in& addr2) {
	return addr1.sin_addr.s_addr == addr2.sin_addr.s_addr && addr1.sin_port == addr2.sin_port;
}

inline bool operator!=(const sockaddr_in& addr1, const sockaddr_in& addr2) {
	return addr1.sin_addr.s_addr != addr2.sin_addr.s_addr || addr1.sin_port != addr2.sin_port;
}

template <>
struct std::hash<sockaddr_in> {
	size_t operator()(const sockaddr_in& addr) const noexcept {
		return static_cast<size_t>(addr.sin_addr.s_addr) << 16 | static_cast<size_t>(addr.sin_port);
	}
};

#endif
