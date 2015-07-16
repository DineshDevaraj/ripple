
#include <cstdio>

#include <unistd.h>
#include <pthread.h>

#include "soketpp.h"
#include "ripple.h"

using namespace Ripple;
   
using namespace Soketpp;

struct DispatchArguments_t
{
   int fdWrt;
   int thread_index;
   Handler_t callback;
};

static void * DispatchRequest(void *args)
{
   int fdReq = 0;
   int fdWrt = 0;
   int thread_index = 0;
   int request_queue[2] = {};

   Handler_t callback;
   DispatchArguments_t *dargs = NULL;

   pipe(request_queue);
   dargs = (DispatchArguments_t *)args;

   fdWrt = dargs->fdWrt;
   callback = dargs->callback;
   
   thread_index = dargs->thread_index;
   
   for(;;)
   {
      write(fdWrt, request_queue + 1, sizeof(int));
      read(request_queue[0], &fdReq, sizeof fdReq);
      callback(thread_index, fdReq);
   }

   return NULL;
}

int Ripple_t::Setup(Handler_t handler_function, const char *host, const char *port, int number_of_thread)
{
   int fdWrt = 0;
   int enroll_queue[2] = {};
   DispatchArguments_t dargs;
   int pipe_ready[number_of_thread];

   pipe(enroll_queue);
   server.Setup(host, port);
   thread_pool = new pthread_t[number_of_thread]();

   nthread = number_of_thread;
   callback = handler_function;
   dargs.fdWrt = enroll_queue[1];
   dargs.callback = handler_function;

   for(int I = 0; I < number_of_thread; I++)
   {
      dargs.thread_index = I;
      pthread_create(thread_pool + I, 0, DispatchRequest, (void *)&dargs);
      read(enroll_queue[0], pipe_ready + I, sizeof(int));
   }

   write(enroll_queue[1], pipe_ready, sizeof(pipe_ready));

   for(;;)
   {
      int cid = -1;
      read(enroll_queue[0], &fdWrt, sizeof fdWrt);
      cid = server.Accept();
      write(fdWrt, &cid, sizeof cid);
      printf("Serving request %d\n", cid);
   }

   return 0;
}
