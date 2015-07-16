
#pragma once

#include <pthread.h>

#include "soketpp.h"

namespace Ripple
{
   using namespace Soketpp;

   /**
    * tid : thread index
    * cid : connection id
    **/
   typedef int (* Handler_t) (int tid, int cid);

   struct Ripple_t : Soketpp::Error_t
   {
      int nthread;
      Handler_t callback;
      pthread_t *thread_pool;
      Soketpp::Server_t server;

	   int Setup(Handler_t handler_function, const char *host, const char *port, int number_of_thread);
   };
}
