
#include "Channel.hpp"

Channel::Channel(const std::string &name)
: _name(name), _topic(""), _inviteOnly(false), _topicOpOnly(false),
  _hasKey(false), _key(""), _hasLimit(false), _limit(0) {}

bool Channel::isMember(int fd) const { return _members.count(fd) != 0; }
bool Channel::isOperator(int fd) const { return _operators.count(fd) != 0; }
bool Channel::isInvited(int fd) const { return _invited.count(fd) != 0; }

void Channel::addMember(int fd) { _members.insert(fd); }
void Channel::removeMember(int fd) {
    _members.erase(fd);
    _operators.erase(fd);
    _invited.erase(fd);
}
void Channel::addOperator(int fd) { _operators.insert(fd); }
void Channel::removeOperator(int fd) { _operators.erase(fd); }
void Channel::addInvite(int fd) { _invited.insert(fd); }
void Channel::clearInvite(int fd) { _invited.erase(fd); }
