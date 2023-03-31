
#include <sys/socket.h>
#include <sys/types.h>

namespace simprpc{

class RPCClient{
public:
  explicit RPCClient(const char* ip, int port);
   

private:
  const char* _remote_ip;
  int _remote_port;
  int _sockfd;
  bool _connected;
  
};

}