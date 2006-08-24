#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include <string>

/**
 * A platform-independent socket API.
 */
class Socket {
public:
	/**
	 * Creates a stream (TCP) socket. Returns -1 on failure.
	 */
	static int socket();
	/**
	 * Closes a socket.
	 * @socket: socket handle
	 */
	static void close(int socket);
	/**
	 * Sets a stream (TCP) socket to perform non-blocking IO.
	 * Returns false on failure.
	 * @socket: socket handle
	 */
	static bool setNonBlocking(int socket);
	/**
	 * Read text from the specified socket. 
	 * Returns false on error.
	 * @socket: socket handle
	 * @s: we save text here
	 * @eof: we set it if occures end of file
	 */
	static bool nbRead(int socket, std::string& s, bool *eof);
	/**
	 * Write text to the specified socket. 
	 * Returns false on error.
	 * @socket: socket handle
	 * @s: where we get text
	 * @bytesSoFar: how many bytes we already wrote
	 */
	static bool nbWrite(int socket, const std::string& s, int *bytesSoFar);
	/**
	 * Connect a socket to a server (from a client)
	 * @socket: socket handle
	 * @host: host name
	 * @port: port number
	 */
	static bool connect(int socket, const std::string& host, int port);
	
	//! Returns last errno
	static int getError();
	//! Returns message corresponding to last error
	static std::string getErrorMsg();
	//! Returns message corresponding to error
	static std::string getErrorMsg(int error);
};



#endif//!_SOCKETS_HPP_
