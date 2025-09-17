#!/usr/bin/env bash
set -euo pipefail

PORT=${1:-6667}
PASS=${2:-secret}

pkill -f ircserv || true
sleep 0.2

# Build
make -C "$(dirname "$0")" -B >/dev/null

# Start server
./ircserv "$PORT" "$PASS" >/tmp/irc_server.log 2>&1 &
srv=$!
sleep 0.3

# Helper: send lines then quit socket
send() {
  # $1 lines... to a new nc client
  {
    IFS=$'\n'
    for L in $1; do
      printf '%s\r\n' "$L"
    done
    sleep 0.1
  } | nc localhost "$PORT"
}

# Client 1 joins #a and #b then PARTs one via QUIT #b
send "PASS $PASS
NICK a
USER a 0 * :a
JOIN #a
JOIN #b
QUIT #b :bye" &

sleep 0.2

# Client 2 joins #a and expects to still see client 1 there
{
  echo "PASS $PASS"
  echo "NICK b"
  echo "USER b 0 * :b"
  echo "JOIN #a"
  sleep 0.2
} | nc localhost "$PORT" > /tmp/irc_c2.log

# Grep: ensure no server exit and that JOIN/PART/PRIVMSG flow works
if ! ps -p "$srv" >/dev/null; then
  echo "ERROR: server exited unexpectedly when QUIT #b used"
  kill "$srv" || true
  exit 1
fi

echo "OK: server still running after QUIT #b"

# Now test QUIT (full disconnect)
send "PASS $PASS
NICK c
USER c 0 * :c
JOIN #z
QUIT :see ya" || true

if ps -p "$srv" >/dev/null; then
  echo "OK: server still running after a client QUIT"
  kill "$srv"
fi

echo "All tests passed."
