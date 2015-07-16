
#include <cstdio>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include <time.h>

#include "soketpp.h"

using namespace Soketpp;

#define MIN_STREAM_LEN 1024

#define SETERR(msg)                    \
({                                     \
   error.line = __LINE__;              \
   error.function = __FUNCTION__;      \
   error.file = __FILE__;              \
   error.fqname = __PRETTY_FUNCTION__; \
   error.message = msg;                \
})

#define THROW(msg)   \
({                   \
   SETERR(msg);      \
   throw error;      \
})
    


/*----------------------------------------------------------------.
 | Connection_t class implementation                              |                    
 `----------------------------------------------------------------*/

#ifndef CONNECTION_CLASS_IMPLEMENTATION

Connection_t::Connection_t()
{
   id = -1;
   
   memset(host, 0, sizeof host);
   memset(port, 0, sizeof port);

   memset(&error, 0, sizeof error);
}

Connection_t::Connection_t(int id)
{
   *this = id;
}

Connection_t::Connection_t(const char *host, const char *port)
{
   Setup(host, port);
}

void Connection_t::operator = (int id)
{
   this->id = id;
   struct sockaddr sa = {};
   socklen_t salen = sizeof sa;

   if(-1 == getpeername(id, &sa, &salen))
      THROW(strerror(errno));
   getnameinfo(&sa, sizeof sa, host, 
         sizeof host, port, sizeof port, 0);
}

void Connection_t::Setup(const char *host, const char *port)
{
   int sd = 0;
   int ret = 0;
   struct addrinfo hint;
   struct addrinfo *resl = NULL;

   memset(&hint, 0, sizeof hint);
   hint.ai_family = AF_UNSPEC;
   hint.ai_socktype = SOCK_STREAM;
   
   if(port && *port) 
      strcpy(this->port, port);
   else THROW("Port cannot be empty");
   
   if(host && *host) 
      strcpy(this->host, host);
   else 
      memset(this->host, 0, sizeof this->host);

   printf("Getaddfinfo %s:%s\n", host, port);

   ret = getaddrinfo(host, port, &hint, &resl);
   if(ret != 0) 
   {
      printf("Getaddrinfo errno %d\n", ret);
      THROW(gai_strerror(ret));
   }

   struct addrinfo *itr = resl;

   for( ; itr; itr = itr->ai_next)
   {
      char szip[INET6_ADDRSTRLEN] = {};

      inet_ntop(itr->ai_family, itr->ai_addr, szip, sizeof szip);

      sd = socket(itr->ai_family, itr->ai_socktype, itr->ai_protocol);
      if(-1 == sd) continue;

      ret = connect(sd, itr->ai_addr, itr->ai_addrlen);
      if(0 == ret) break;

      puts(szip);
   }

   freeaddrinfo(resl);
   if(NULL == itr) THROW("Not able to connect");
   id = sd;
}

void Connection_t::Close()
{
   close(id);
}

long Connection_t::Send(const char *str)
{
   return Send((const Byte *)str, strlen(str));
}

long Connection_t::Send(const Byte *hay, long len)
{
   long sl = 0;  /* send length */
   long tl = 0;  /* total length */

   while(tl < len)
   {
      sl = send(id, hay + tl, len - tl, 0);
      if(-1 == sl or 0 == sl) return tl;
      tl += sl;
   }

   return tl;
}

long Connection_t::Send(const Byte *hay, long len, int timeout)
{
   int epfd;
   
   long tl = 0;
   long sl = 0;

   struct epoll_event epev = {};

   epev.data.fd = id;
   epfd = epoll_create(1);
   epev.events = EPOLLIN | EPOLLET;
   epoll_ctl(epfd, EPOLL_CTL_ADD, id, &epev);
   
   struct timespec st = {};            /* start time */
   struct timespec ct = {};            /* current time */
   long tr = timeout<0?0:timeout;      /* time remaining */
   clock_gettime(CLOCK_REALTIME, &st);

   while(tl < len)
   {
      if(epoll_wait(epfd, &epev, 1, tr) != 1)
         return 0;

      for(;;)
      {
         sl = send(id, hay + tl, len - tl, MSG_DONTWAIT);
         if(0 == sl) return tl;
         else if(-1 == sl)
         {
            if(EAGAIN == errno)
            {
               clock_gettime(CLOCK_REALTIME, &ct);
               long te = (ct.tv_nsec - st.tv_nsec) / 1000000;
               tr -= te>tr?tr:te;
               break;
            }
            else return tl;
         }
         tl += sl;
      }
   }

   return tl;
}
      
long Connection_t::Send(const char *str, const char chdlm)
{
   return Send((const Byte *)str, (long)(strchr(str, chdlm) - str));
}

long Connection_t::Send(const char *str, const char *strdlm)
{
   return Send((const Byte *)str, (long)(strstr(str, strdlm) - str));
}

long Connection_t::Send(const char *str, const char chdlm, int timeout)
{
   return Send((const Byte *)str, long(strchr(str, chdlm) - str), timeout);
}
      
long Connection_t::Send(const char *str, const char *strdlm, int timeout)
{
   return Send((const Byte *)str, long(strstr(str, strdlm) - str), timeout);
}
   
long Connection_t::Recv(Byte *hay)
{
   int tl = 0;
   int rl = 0;

   for(;;)
   {
      rl = recv(id, hay + tl, MIN_STREAM_LEN, 0);
      if(-1 == rl or 0 == rl) return tl;
      tl += rl;
   }

   return tl;
}
      
long Connection_t::Recv(Byte *hay, long len)
{
   int tl = 0;
   int rl = 0;

   while(tl < len)
   {
      rl = recv(id, hay + tl, len - tl, 0);
      if(-1 == rl or 0 == rl) return tl;
      tl += rl;
   }

   return tl;
}
      
long Connection_t::Recv(Byte *hay, long len, int timeout)
{
   int epfd = 0;

   long tl = 0;
   long rl = 0;

   struct epoll_event epev = {};

   epev.data.fd = id;
   epfd = epoll_create(1);
   epev.events = EPOLLIN | EPOLLET;
   epoll_ctl(epfd, EPOLL_CTL_ADD, id, &epev);

   struct timespec st = {};            /* start time */
   struct timespec ct = {};            /* current time */
   long tr = timeout<0?0:timeout;      /* time remaining */
   clock_gettime(CLOCK_REALTIME, &st);

   while(tl < len)
   {
      if(epoll_wait(epfd, &epev, 1, tr) != 1)
         return tl;

      for(;;)
      {
         rl = recv(id, hay + tl, len - tl, MSG_DONTWAIT);
         if(0 == rl) return tl;
         else if(-1 == rl)
         {
            if(EAGAIN == errno)
            {
               clock_gettime(CLOCK_REALTIME, &ct);
               long te = (ct.tv_nsec - st.tv_nsec) / 1000000;
               tr -= te>tr?tr:te;
               break;
            }
            else return tl;
         }
         tl += rl;
      }
   }

   return tl;
}
      
long Connection_t::Recv(char *str, const char chdlm)
{
   int tl = 0;
   int rl = 0;

   for(;;)
   {
      rl = recv(id, str + tl, MIN_STREAM_LEN, 0);
      if(-1 == rl or 0 == rl) return tl;
      else if(strchr(str + tl, chdlm)) 
         return tl + rl;
      tl += rl;
   }

   return tl;
}

long Connection_t::Recv(char *str, const char *strdlm)
{
   int tl = 0;
   int rl = 0;

   for(;;)
   {
      rl = recv(id, str + tl, MIN_STREAM_LEN, 0);
      if(-1 == rl or 0 == rl) return tl;
      else if(strstr(str + tl, strdlm)) 
         return tl + rl;
      tl += rl;
   }

   return tl;
}
      
long Connection_t::Recv(char *str, const char chdlm, int timeout)
{
   int epfd = 0;

   long tl = 0;
   long rl = 0;

   struct epoll_event epev = {};

   epev.data.fd = id;
   epfd = epoll_create(1);
   epev.events = EPOLLIN | EPOLLET;
   epoll_ctl(epfd, EPOLL_CTL_ADD, id, &epev);

   struct timespec st = {};            /* start time */
   struct timespec ct = {};            /* current time */
   long tr = timeout<0?0:timeout;      /* time remaining */
   clock_gettime(CLOCK_REALTIME, &st);

   for(;;)
   {
      if(epoll_wait(epfd, &epev, 1, tr) != 1)
         return tl;

      for(;;)
      {
         rl = recv(id, str + tl, MIN_STREAM_LEN, MSG_DONTWAIT);
         if(0 == rl) return tl;
         else if(-1 == rl)
         {
            if(EAGAIN == errno)
            {
               clock_gettime(CLOCK_REALTIME, &ct);
               long te = (ct.tv_nsec - st.tv_nsec) / 1000000;
               tr -= te>tr?tr:te;
               break;
            }
            else return tl;
         }
         else if(strchr(str + tl, chdlm))
            return tl + rl;
         tl += rl;
      }
   }

   return tl;
}

long Connection_t::Recv(char *str, const char *strdlm, int timeout)
{
   int epfd = 0;

   long tl = 0;
   long rl = 0;

   struct epoll_event epev = {};

   epev.data.fd = id;
   epfd = epoll_create(1);
   epev.events = EPOLLIN | EPOLLET;
   epoll_ctl(epfd, EPOLL_CTL_ADD, id, &epev);

   struct timespec st = {};            /* start time */
   struct timespec ct = {};            /* current time */
   long tr = timeout<0?0:timeout;      /* time remaining */
   clock_gettime(CLOCK_REALTIME, &st);

   for(;;)
   {
      if(epoll_wait(epfd, &epev, 1, tr) != 1)
         return tl;

      for(;;)
      {
         rl = recv(id, str + tl, MIN_STREAM_LEN, MSG_DONTWAIT);
         if(0 == rl) return tl;
         else if(-1 == rl)
         {
            if(EAGAIN == errno)
            {
               clock_gettime(CLOCK_REALTIME, &ct);
               long te = (ct.tv_nsec - st.tv_nsec) / 1000000;
               tr -= te>tr?tr:te;
               break;
            }
            else return tl;
         }
         else if(strstr(str + tl, strdlm))
            return tl + rl;
         tl += rl;
      }
   }

   return tl;
}

Connection_t::~Connection_t() 
{
   puts("Connection closed");
   close(id);
}

#endif



/*----------------------------------------------------------------.
 | Server_t class implementation                                  |                    
 `----------------------------------------------------------------*/

#ifndef SERVER_CLASS_DEFENITION

Server_t::Server_t()
{
   id = -1;

   memset(host, 0, sizeof host);
   memset(port, 0, sizeof port);

   memset(&error, 0, sizeof error);
}

int Server_t::Accept()
{
   int cid = -1;
   struct sockaddr sa;
   unsigned int salen = sizeof sa;
   cid = accept(id, &sa, &salen);
   return cid;
}

Server_t::Server_t(const char *host, const char *port)
{
   Setup(host, port);
}

void Server_t::Setup(const char *host, const char *port)
{
   int sd = 0;
   int ret = 0;
   int optval = 1;

   struct addrinfo hint;
   struct addrinfo *resl = NULL;

   if(port && *port) 
      strcpy(this->port, port);
   else THROW("Port cannot be empty");
   
   if(host && *host) 
      strcpy(this->host, host);
   else 
      memset(this->host, 0, sizeof this->host);

   memset(&hint, 0, sizeof hint);
   hint.ai_flags = AI_PASSIVE;
   hint.ai_family = AF_UNSPEC;
   hint.ai_socktype = SOCK_STREAM;

   printf("Getaddfinfo %s:%s\n", host, port);

   ret = getaddrinfo(host, port, &hint, &resl);
   if(ret != 0) 
   {
      printf("Getaddrinfo errno %d\n", ret);
      THROW(gai_strerror(ret));
   }

   struct addrinfo *itr = resl;
   for( ; itr; itr = itr->ai_next)
   {
      char szip[INET6_ADDRSTRLEN] = {};

      inet_ntop(itr->ai_family, itr->ai_addr, szip, sizeof szip);

      sd = socket(itr->ai_family, itr->ai_socktype, itr->ai_protocol);
      if(-1 == sd) continue;

      ret = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof optval);
      if(ret != 0) THROW(strerror(errno));
      
      ret = bind(sd, itr->ai_addr, itr->ai_addrlen);
      if(0 == ret) break;

      puts(szip);
   }

   freeaddrinfo(resl); 
   resl = NULL;
   
   if(NULL == itr) THROW("Not able to connect");
   
   listen(sd, 10);
   id = sd;
}

#endif
