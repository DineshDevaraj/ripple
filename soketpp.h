
#pragma once

namespace Soketpp
{
   typedef unsigned char Byte;

   struct Error_t
   {
      long        line;
      const char *file;
      const char *function;
      const char *message;
      const char *fqname;     /* fully qualified function name */
   };

   struct Sokinfo_t
   {  
      int id;

      char port[8];
      char host[46];

      Error_t error;
   };

   struct Connection_t : Sokinfo_t
   {
      Connection_t();

      Connection_t(int id); 

      Connection_t(const char *host, const char *port);

      void Close();

      void Shutdown();
      
      void operator = (int id);
      
      void Setup(const char *host, const char *port);

      /* NULL terminated string */
      long Send(const char *str);

      /* block untill len byte is send */
      long Send(const Byte *hay, long len);

      long Send(const char *str, const char chdlm);

      /* send untill strdlm reached */
      long Send(const char *str, const char *strdlm);

      /* send byte <= len before timeout and return */
      long Send(const Byte *hay, long len, int timeout);

      long Send(const char *str, const char chdlm, int timeout);

      /* send untill strdlm reached with timeout */
      long Send(const char *str, const char *strdlm, int timeout);

      /* recv till connection close */
      long Recv(Byte *hay);

      /* block untill len byte recived */
      long Recv(Byte *hay, long len);

      long Recv(char *str, const char chdlm);

      long Recv(char *str, const char *strdlm);

      long Recv(Byte *hay, long len, int timeout);

      long Recv(char *str, const char chdlm, int timeout);

      long Recv(char *str, const char *strdlm, int timeout);

      ~Connection_t();
   };

   struct Server_t : Sokinfo_t
   {
      Server_t();

      int Accept();

      Server_t(const char *host, const char *port);

      void Setup(const char *host, const char *port);
   };
}
