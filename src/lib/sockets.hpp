#ifndef _SOCKETS_HPP_
#define _SOCKETS_HPP_

#include <glib.h>
#include <string>
#include <map>
#ifndef _WIN32
#  include <netdb.h>
#else
#  include <WinSock.h>
typedef unsigned long in_addr_t;
#endif

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

	typedef void (*on_resolved_func)(gpointer data, bool resolved, in_addr_t sa);
	static void resolve(std::string& host, gpointer data, on_resolved_func func);

	//! Connect a socket to a server (from a client)
	typedef void (*on_connected_func)(gpointer data, bool succeeded);
	static void connect(int socket, in_addr_t sa, int port, gpointer data, on_connected_func func);


	//! Returns last errno
	static int get_error_code();

	//! Returns message corresponding to last error
	static std::string get_error_msg();
private:
	struct DnsQueryData {
		std::string host;
		gpointer data;
		on_resolved_func func;
		bool resolved;
		in_addr_t sa;
    };
    static gpointer dns_thread(gpointer data);
    static gboolean dns_main_thread_cb(gpointer data);
	static std::map<std::string, in_addr_t> dns_map;
	struct ConnectData {
		int sd;
		in_addr_t sa;
		int port;
		gpointer data;
		on_connected_func func;
		bool succeeded;
    };
    static gpointer connect_thread(gpointer data);
    static gboolean connect_main_thread_cb(gpointer data);
};


#endif//!_SOCKETS_HPP_
