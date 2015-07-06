/*
 * Copyright 2011 kubtek <kubtek@mail.com>
 *
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif


#include <cstdio>
#include <cstring>
#include <glib.h>

#if defined(_WIN32)

# include <winsock2.h>

#if defined(_MSC_VER)
#pragma comment(lib,"WS2_32.lib")
#endif

# define EINPROGRESS	WSAEINPROGRESS
# define EWOULDBLOCK	WSAEWOULDBLOCK
# define ETIMEDOUT	    WSAETIMEDOUT
# define EAGAIN			WSAEWOULDBLOCK
# define EINTR			WSAEINTR
#else
extern "C" {
# include <unistd.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <cerrno>
# include <fcntl.h>
}
#endif  // _WIN32

#include "sockets.h"

std::map<std::string, in_addr_t> Socket::dns_map;

#if defined(_WIN32)
  
static void initWinSock()
{
  static bool wsInit = false;
  if (! wsInit)
  {
    WORD wVersionRequested = MAKEWORD( 2, 0 );
    WSADATA wsaData;
    WSAStartup(wVersionRequested, &wsaData);
    wsInit = true;
  }
}

#else

#define initWinSock()

#endif // _WIN32


// These errors are not considered fatal for an IO operation; the operation will be re-tried.

static inline bool
nonFatalError()
{
  int err = Socket::get_error_code();
  return (err == EINPROGRESS || err == EAGAIN || err == EWOULDBLOCK || err == EINTR);
}



int
Socket::socket()
{
  initWinSock();
  return (int) ::socket(AF_INET, SOCK_STREAM, 0);
}


void
Socket::close(int fd)
{
#if defined(_WIN32)
  closesocket(fd);
#else
  ::close(fd);
#endif // _WIN32
}




bool
Socket::set_non_blocking(int fd)
{
#if defined(_WIN32)
  unsigned long flag = 1;
  return (ioctlsocket((SOCKET)fd, FIONBIO, &flag) == 0);
#else
  return (fcntl(fd, F_SETFL, O_NONBLOCK) == 0);
#endif // _WIN32
}


bool
Socket::set_reuse_addr(int fd)
{
  // Allow this port to be re-bound immediately so server re-starts are not delayed
  int sflag = 1;
  return (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&sflag, sizeof(sflag)) == 0);
}


// Bind to a specified port
bool 
Socket::bind(int fd, int port)
{
  struct sockaddr_in saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  saddr.sin_port = htons((u_short) port);
  return (::bind(fd, (struct sockaddr *)&saddr, sizeof(saddr)) == 0);
}


// Set socket in listen mode
bool Socket::listen(int fd, int backlog)
{
  return (::listen(fd, backlog) == 0);
}


int Socket::accept(int fd)
{
  struct sockaddr_in addr;
#if defined(_WIN32)
  int
#else
  socklen_t
#endif
    addrlen = sizeof(addr);

  return (int) ::accept(fd, (struct sockaddr*)&addr, &addrlen);
}

gboolean Socket::dns_main_thread_cb(gpointer data)
{
    DnsQueryData *query_data = (DnsQueryData *)data;
    if (query_data->resolved) {
		dns_map[query_data->host] = query_data->sa;
	}
    query_data->func(query_data->data, query_data->resolved, query_data->sa);
    delete query_data;
    return FALSE;
}

gpointer Socket::dns_thread(gpointer data)
{
    DnsQueryData *query_data = (DnsQueryData *)data;
    struct  hostent *phost;
#ifndef _WIN32    
#ifdef HAVE_GETHOSTBYNAME_R
    struct  hostent hostinfo;
    char buf[1024];
    int ret, ret2;
    ret2 = gethostbyname_r(query_data->host.c_str(), &hostinfo, buf,
        sizeof(buf), &phost, &ret);

    if (ret2 == 0 && ret == 0 && phost != NULL) {   // classical code!
        query_data->sa = ((in_addr*)(hostinfo.h_addr))->s_addr;
        query_data->resolved = true;
    } else {
        query_data->resolved = false;
    }
#else
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
	g_static_mutex_lock (&mutex);
	phost = gethostbyname(query_data->host.c_str());
	if (phost) {
		query_data->sa = ((in_addr*)(phost->h_addr))->s_addr;
		query_data->resolved = true;
	} else {
		query_data->resolved = false;
	}
	g_static_mutex_unlock (&mutex);
#endif
#else
	//static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
	//g_static_mutex_lock (&mutex);
	if (isalpha(query_data->host[0])) {
		phost = gethostbyname(query_data->host.c_str());
	} else {
		unsigned int addr;
		addr = inet_addr(query_data->host.c_str());
		phost = gethostbyaddr((char *)&addr, 4, AF_INET);
	}
	if (phost) {
		query_data->sa = ((in_addr*)(phost->h_addr))->s_addr;
		query_data->resolved = true;
	} else {
		query_data->resolved = false;
	}
	//g_static_mutex_unlock (&mutex);
#endif                     
    /* back to main thread */
    g_idle_add(dns_main_thread_cb, query_data);
    return NULL;
}

void Socket::resolve(std::string& host, gpointer data, on_resolved_func func)
{
	initWinSock();
	std::map<std::string, in_addr_t>::iterator iter;
	iter = dns_map.find(host);
	if (iter != dns_map.end()) {
		func(data, true, iter->second);
		return;
	}
	DnsQueryData *query_data = new DnsQueryData();
	query_data->host = host;
	query_data->data = data;
	query_data->func = func;
	g_thread_unref(g_thread_new("dns_thread", dns_thread, query_data));
}

void Socket::connect(int socket, in_addr_t sa, int port, gpointer data, on_connected_func func)
{
	ConnectData *connect_data = new ConnectData();
	connect_data->sd = socket;
	connect_data->sa = sa;
	connect_data->port = port;
	connect_data->data = data;
	connect_data->func = func;
	g_thread_unref(g_thread_new("connect_thread", connect_thread, connect_data));
}

gpointer Socket::connect_thread(gpointer data)
{
    ConnectData *connect_data = (ConnectData *)data;
	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = connect_data->sa;
	saddr.sin_port = htons((u_short) connect_data->port);

	// For asynch operation, this will return EWOULDBLOCK (windows) or
	// EINPROGRESS (linux) and we just need to wait for the socket to be writable...
	int result = ::connect(connect_data->sd, (struct sockaddr *)&saddr, sizeof(saddr));
	connect_data->succeeded = (result == 0);

    /* back to main thread */
    g_idle_add(connect_main_thread_cb, connect_data);
    return NULL;
}

gboolean Socket::connect_main_thread_cb(gpointer data)
{
    ConnectData *connect_data = (ConnectData *)data;
	connect_data->func(connect_data->data, connect_data->succeeded);
    delete connect_data;
    return FALSE;
}
    
// Read available text from the specified socket. Returns false on error.
bool Socket::nb_read(int fd, std::string& s, bool *eof)
{
  const int READ_SIZE = 4096;   // Number of bytes to attempt to read at a time
  char readBuf[READ_SIZE];

  bool wouldBlock = false;
  *eof = false;

  while ( ! wouldBlock && ! *eof) {
#if defined(_WIN32)
    int n = recv(fd, readBuf, READ_SIZE-1, 0);
#else
    int n = read(fd, readBuf, READ_SIZE-1);
#endif
    g_debug("Socket::nbRead: read/recv returned %d.", n);


    if (n > 0) {
      readBuf[n] = 0;
      s.append(readBuf, n);
    } else if (n == 0) {
      *eof = true;
    } else if (nonFatalError()) {
      wouldBlock = true;
    } else {
      return false;   // Error
    }
  }
  return true;
}


// Write text to the specified socket. Returns false on error.
bool Socket::nb_write(int fd, std::string& s, int *bytesSoFar)
{
  int nToWrite = int(s.length()) - *bytesSoFar;
  char *sp = const_cast<char*>(s.c_str()) + *bytesSoFar;
  bool wouldBlock = false;

  while ( nToWrite > 0 && ! wouldBlock ) {
#if defined(_WIN32)
    int n = send(fd, sp, nToWrite, 0);
#else
    int n = write(fd, sp, nToWrite);
#endif
    g_debug("Socket::nbWrite: send/write returned %d.", n);

    if (n > 0) {
      sp += n;
      *bytesSoFar += n;
      nToWrite -= n;
    } else if (nonFatalError()) {
      wouldBlock = true;
    } else {
      return false;   // Error
    }
  }
  return true;
}


// Returns last errno
int Socket::get_error_code()
{
#if defined(_WIN32)
  return WSAGetLastError();
#else
  return errno;
#endif
}


// Returns message corresponding to last errno
std::string Socket::get_error_msg()
{
//Actually works on windows, but may be better use FormatMessage?
  return strerror(get_error_code());

}

