
#ifndef CHANNEL_HPP
#define CHANNEL_HPP

// A Channel tracks its members, operators and various modes required by subject.

#include <string>
#include <set>
#include <map>

class Channel {
    std::string _name;        // "#name"
    std::string _topic;       // current topic, may be empty
    // Modes per the subject: i, t, k, o, l (channel-level)
    bool        _inviteOnly;  // +i
    bool        _topicOpOnly; // +t
    bool        _hasKey;      // +k
    std::string _key;         // key if +k
    bool        _hasLimit;    // +l
    size_t      _limit;       // user limit if +l
    // Fds of members and operators
    std::set<int> _members;
    std::set<int> _operators;
    std::set<int> _invited;   // for +i logic
public:
    Channel(const std::string &name);
    const std::string &name() const { return _name; }
    const std::string &topic() const { return _topic; }
    void setTopic(const std::string &t) { _topic = t; }
    bool inviteOnly() const { return _inviteOnly; }
    bool topicOpOnly() const { return _topicOpOnly; }
    bool hasKey() const { return _hasKey; }
    const std::string &key() const { return _key; }
    bool hasLimit() const { return _hasLimit; }
    size_t limit() const { return _limit; }

    void setInviteOnly(bool v) { _inviteOnly = v; }
    void setTopicOpOnly(bool v) { _topicOpOnly = v; }
    void setKey(const std::string &k) { _hasKey = true; _key = k; }
    void clearKey() { _hasKey = false; _key.clear(); }
    void setLimit(size_t l) { _hasLimit = true; _limit = l; }
    void clearLimit() { _hasLimit = false; _limit = 0; }

    const std::set<int> &members() const { return _members; }
    const std::set<int> &operators() const { return _operators; }
    const std::set<int> &invited() const { return _invited; }

    bool isMember(int fd) const;
    bool isOperator(int fd) const;
    bool isInvited(int fd) const;
    void addMember(int fd);
    void removeMember(int fd);
    void addOperator(int fd);
    void removeOperator(int fd);
    void addInvite(int fd);
    void clearInvite(int fd);
    size_t memberCount() const { return _members.size(); }
};

#endif
