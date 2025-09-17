
# ft_irc (minimal & clean)

This is a *simple*, human-readable IRC server that passes the **mandatory** checks of the 42 `ft_irc` correction sheet.  
Design goals:
- C++98 only, no external libs
- Non-blocking sockets + **single** `poll()` for listen/read/write (no busy loops; no reliance on `errno` for flow)
- Implement only the subject-required features, plainly

## Build

```bash
make
./ircserv 6667 mypass
```

## Reference client

Use any standard client (e.g., `irssi`, `weechat`, or `HexChat`). You can also test with `nc`.

## Quick test with nc

Terminal A:
```bash
./ircserv 6667 mypass
```

Terminal B:
```bash
nc -C 127.0.0.1 6667
PASS mypass
NICK alice
USER alice 0 * :Alice
JOIN #test
PRIVMSG #test :hello world
```

Terminal C:
```bash
nc -C 127.0.0.1 6667
PASS mypass
NICK bob
USER bob 0 * :Bob
JOIN #test
# You should see alice's messages.
```

Partial line aggregation per subject:
```
nc -C 127.0.0.1 6667
com^Dman^Dd
```
(That sends `command\n` in 3 fragments; the server aggregates and parses lines when a newline arrives.)

## Required features (implemented)

- Multiple clients via TCP, non-blocking, one `poll()` for listen/read/write
- PASS/NICK/USER auth → welcome on full registration
- JOIN channels, broadcast JOIN, topic/names
- PRIVMSG `<nick>` and `#channel`
- Operators vs regular users:
  - First user in a channel becomes operator
  - MODE `+i -i`, `+t -t`, `+k <key> -k`, `+o <nick> -o <nick>`, `+l <n> -l`
  - INVITE, KICK, TOPIC (view & set; subject scope)
- PING/PONG minimal handling
- Graceful QUIT/close; removes user from channels

## Mandatory correction alignment

- **Exactly one** `poll()` in the main loop; *all* `accept`, `recv`, and `send` are performed **only after** that `poll()` indicates readiness.
- `fcntl(fd, F_SETFL, O_NONBLOCK);` and **no other flags**.
- We **never** loop on `errno == EAGAIN` to trigger action. If a call would block, we simply wait for the next `poll()` event.
- Server listens on **all interfaces** on the provided port.
- Handles partial input; aggregates until newline; processes command lines safely.

## Notes

- Code is intentionally straightforward—no templates or over-engineering. Names and files are small and readable.
- Numeric replies are minimal, just enough for required flows.
- Server name shown in prefixes is `ft_irc.min`.
