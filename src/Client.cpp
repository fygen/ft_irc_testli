
#include "Client.hpp"

Client::Client(int fd)
: _fd(fd), _inbuf(""), _outbuf(""), _nick(""), _user(""), _realname(""),
  _passOk(false), _registered(false) {}
