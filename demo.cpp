
#include <cstdio>
#include <unistd.h>

#include "ripple.h"

using namespace Ripple;

int HandleConnection(int tid, int cid)
{
   Connection_t con(cid);
   printf("Received request : %s %s %d %d\n", con.host, con.port, cid, tid);
   con.Send("Welcome to ripple\n");
   sleep(120 - tid);
}

int main()
{
   Ripple_t lb;
	lb.Setup(HandleConnection, "localhost", "8080", 32);
	return 0;
}
