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

#endif
