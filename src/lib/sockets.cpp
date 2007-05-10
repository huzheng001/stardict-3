#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif


#include <cstdio>
#include <cstring>
#include <glib.h>

#if defined(_WIN32)

# include <winsock2.h>

# define EINPROGRESS	WSAEINPROGRESS
# define EWOULDBLOCK	WSAEWOULDBLOCK
# define ETIMEDOUT	    WSAETIMEDOUT
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

#include "sockets.hpp"

std::map<std::string, Socket::ResolveInfo> Socket::dns_map;

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
  g_debug("Socket::close: fd %d.", fd);
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
	ResolveInfo info;
	info.hostinfo = query_data->hostinfo;
	dns_map[query_data->host] = info;

        query_data->func(query_data->data, &(query_data->hostinfo));
    }
    delete query_data;
    return FALSE;
}

gpointer Socket::dns_thread(gpointer data)
{
    DnsQueryData *query_data = (DnsQueryData *)data;
    struct  hostent *phost;
#ifndef _WIN32    
    char buf[1024];
    int ret;
    if (!gethostbyname_r(query_data->host.c_str(), &query_data->hostinfo, buf,
        sizeof(buf), &phost, &ret)) {
        query_data->resolved = true;
    } else {
        query_data->resolved = false;
    }
#else
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
	g_static_mutex_lock (&mutex);
	phost = gethostbyname(query_data->host.c_str());
	if (phost) {
		query_data->hostinfo = *phost;
		query_data->resolved = true;
	} else {
		query_data->resolved = false;
	}
	g_static_mutex_unlock (&mutex);
#endif                     
    /* back to main thread */
    g_idle_add(dns_main_thread_cb, query_data);
    return NULL;
}

void Socket::resolve(std::string& host, gpointer data, on_resolved_func func)
{
	std::map<std::string, ResolveInfo>::iterator iter;
	iter = dns_map.find(host);
	if (iter != dns_map.end()) {
		func(data, &(iter->second.hostinfo));
		return;
	}
    DnsQueryData *query_data = new DnsQueryData();
    query_data->host = host;
    query_data->data = data;
    query_data->func = func;
    g_thread_create(dns_thread, query_data, FALSE, NULL);
}
    
// Connect a socket to a server (from a client)
bool
Socket::connect(int fd, struct hostent *hp, int port)
{
  struct sockaddr_in saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;

  saddr.sin_family = hp->h_addrtype;
  memcpy(&saddr.sin_addr, hp->h_addr, hp->h_length);
  saddr.sin_port = htons((u_short) port);

  // For asynch operation, this will return EWOULDBLOCK (windows) or
  // EINPROGRESS (linux) and we just need to wait for the socket to be writable...
  int result = ::connect(fd, (struct sockaddr *)&saddr, sizeof(saddr));
  return result == 0 || nonFatalError();
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

