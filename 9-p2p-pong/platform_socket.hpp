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

	struct SocketSession {
		bool initialized = true;
	};

	inline int socket_error() {
		return errno;
	}
#endif

sockaddr_in create_sockaddr(in_addr_t addr, in_port_t port) {
    sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons(port);
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

	template <typename T>
	int setsockopt(int opt_name, const T& opt_val) {
		if (!is_available()) return -1;
		return ::setsockopt(_socket, SOL_SOCKET, opt_name, &opt_val, sizeof(opt_val));
	}

	int connect(const sockaddr_in& address) {
		if (!is_available()) return -1;
		return ::connect(_socket, sockaddr_cast(&address), sizeof(address));
	}

	int bind(const sockaddr_in& address) {
		if (!is_available()) return -1;
		return ::bind(_socket, sockaddr_cast(&address), sizeof(address));
	}

	int listen(int nconn) {
		if (!is_available()) return -1;
		return ::listen(_socket, nconn);
	}

	int getsockname(sockaddr_in& address) {
		socklen_t socklen = sizeof(address);
		return ::getsockname(_socket, sockaddr_cast(&address), &socklen);
	}

	int sendto(const void* buf, size_t buf_size, const sockaddr_in& address, int flags = 0) {
		return ::sendto(_socket, buf, buf_size, flags, sockaddr_cast(&address), sizeof(address));
	}

	int recvfrom(void* buf, size_t buf_size, sockaddr_in& address, int flags = 0) {
		socklen_t socklen = sizeof(address);
		return ::recvfrom(_socket, buf, buf_size, flags, sockaddr_cast(&address), &socklen);
	}

	int send(const void* buf, size_t buf_size, int flags = 0) {
		return ::send(_socket, buf, buf_size, flags);
	}

	int recv(void* buf, size_t buf_size, int flags = 0) {
		return ::recv(_socket, buf, buf_size, flags);
	}

	SocketResource accept(sockaddr_in& address) {
		socklen_t socklen = sizeof(address);
		SocketResource client(::accept(_socket, sockaddr_cast(&address), &socklen));
		return client;
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


#endif
