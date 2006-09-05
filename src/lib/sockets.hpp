#ifndef _SOCKETS_HPP_
#define _SOCKETS_HPP_

#include <string>

//! A platform-independent socket API.
class Socket {
public:
	//! Creates a stream (TCP) socket. Returns -1 on failure.
	static int socket();

	//! Closes a socket.
	static void close(int socket);

	//! Sets a stream (TCP) socket to perform non-blocking IO. Returns false on failure.
	static bool set_non_blocking(int socket);

	//! Read text from the specified socket. Returns false on error.
	static bool nb_read(int socket, std::string& s, bool *eof);

	//! Write text to the specified socket. Returns false on error.
	static bool nb_write(int socket, std::string& s, int *bytesSoFar);

	// The next four methods are appropriate for servers.

	//! Allow the port the specified socket is bound to to be re-bound immediately so 
	//! server re-starts are not delayed. Returns false on failure.
	static bool set_reuse_addr(int socket);

	//! Bind to a specified port
	static bool bind(int socket, int port);

	//! Set socket in listen mode
	static bool listen(int socket, int backlog);

	//! Accept a client connection request
	static int accept(int socket);


	//! Connect a socket to a server (from a client)
	static bool connect(int socket, std::string& host, int port);


	//! Returns last errno
	static int get_error_code();

	//! Returns message corresponding to last error
	static std::string get_error_msg();
};


#endif//!_SOCKETS_HPP_
