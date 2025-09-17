#!/usr/bin/env bash
# run_tests.sh

set -euo pipefail

PORT="${1:-6667}"            # Arg1: port (yoksa 6667)
PASS="${2:-password}"        # Arg2: server parolası (yoksa 'password')
# ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="./tests/output/"            # Çıktı klasörü

mkdir -p "$OUT"

echo "Starting test runner... PORT: $PORT, PASSWORD: $PASS"

# Eski süreçleri temizle (sessizce)
pkill -f '[./]ircserv' >/dev/null 2>&1 || true
rm -rf "$OUT"/*.txt

# Sunucuyu başlat ve logla
echo "Starting server..."
"./ircserv" "$PORT" "$PASS" >"$OUT/server_output.txt" 2>&1 &
SERVER_PID=$!
trap 'echo "Shutting down server ($SERVER_PID)"; kill $SERVER_PID >/dev/null 2>&1 || true' EXIT

# Port hazır olana kadar bekle (max ~5 sn)
for i in {1..50}; do
  if nc -z localhost "$PORT" 2>/dev/null; then
    echo "Server is up."
    break
  fi
  sleep 0.1
  if ! kill -0 "$SERVER_PID" 2>/dev/null; then
     echo "Server crashed; logs:"
     tail -n +1 "$OUT/server_output.txt"
     exit 1
  fi
  if [[ $i -eq 50 ]]; then
     echo "Server didn't open port $PORT in time."
     exit 1
  fi
done

# Tek seferde komut gönderip bağlantıyı kapatan scripted client
run_client() {
  # Parametreler: nick, name, user, channel, output_file
  local nick="$1" name="$2" user="$3" channel="$4" out="$5"
  {
    sleep 0.2
    printf 'PASS %s\r\n' "$PASS"
    printf 'NICK %s\r\n' "$nick"
    printf 'USER %s 0 * :%s\r\n' "$user" "$user"
    printf 'JOIN %s\r\n' "$channel"
    sleep 5
    printf 'PRIVMSG %s :%s says hello!\r\n' "$channel" "$name"
  } | nc localhost "$PORT" >"$out" 2>&1 &
}

# Kanal konusu belirleyen istemci
topic_set_client() {
  local nick="$1" channel="$2" out="$3"
  {
    printf 'PASS %s\r\n' "$PASS"
    printf 'NICK %s\r\n' "$nick"
    printf 'USER %s 0 * :%s\r\n' "$nick" "$nick"
    printf 'JOIN %s\r\n' "$channel"
    printf 'TOPIC %s : TOPIC_SET_WORKS\r\n' "$TOPIC_CHANNEL"
    sleep 1
  } | nc localhost "$PORT" >"$out" 2>&1 &
}

# Kanal davetiyesi gönderen istemci
inviter_client() {
  local nick="$1" channel="$2" out="$3" invitee="$4"
  {
    printf 'PASS %s\r\n' "$PASS"
    printf 'NICK %s\r\n' "$nick"
    printf 'USER %s 0 * :%s\r\n' "$nick" "$nick"
    printf 'JOIN %s\r\n' "$channel"
    printf 'MODE %s +i\r\n' "$channel"
    printf 'PRIVMSG %s :Hello, I will invite %s!\r\n' "$channel" "$invitee"
    sleep 0.3
    printf 'INVITE %s %s\r\n' "$invitee" "$channel"
    sleep 1
  } | nc localhost "$PORT" >"$out" 2>&1 &
}

# Kanal davetiyesi alıp katılmaya çalışan istemci
joiner_client() {
  local nick="$1" channel="$2" out="$3"
  {
    printf 'PASS %s\r\n' "$PASS"
    printf 'NICK %s\r\n' "$nick"
    printf 'USER %s 0 * :%s\r\n' "$nick" "$nick"
    sleep 1
    printf 'JOIN %s\r\n' "$channel"
    sleep 1
  } | nc localhost "$PORT" >"$out" 2>&1 &
}

# İki istemciyi bağla
INVITER="inviter_user"
INVITEE="invitee_user"
TOPIC_SETTER="topic_setter_user"
CLIENT1="Client1"
CLIENT2="Client2"
CLIENT3="Client3"
CLIENT4="Client4"
ALICE="alice"
CHANNEL="#test"
TOPIC_CHANNEL="#topic_test"
INVITE_CHANNEL="#invite"

# INVITE TESTI
# TOPIC TESTI
# JOINER TESTI 
# INVITER JOINER TESTI


inviter_client "$INVITER" "$INVITE_CHANNEL" "$OUT/$INVITER.txt" "$INVITEE"
topic_set_client "$TOPIC_SETTER" "$TOPIC_CHANNEL" "$OUT/$TOPIC_SETTER.txt"

run_client "$CLIENT1" "$CLIENT1"  "Test User"  "$CHANNEL" "$OUT/$CLIENT1.txt"
run_client "$CLIENT2" "$CLIENT2" "Test User2" "$CHANNEL" "$OUT/$CLIENT2.txt"

run_client "$CLIENT3" "$CLIENT3" "Test User3" "$TOPIC_CHANNEL" "$OUT/$CLIENT3.txt"
run_client "$CLIENT4" "$CLIENT4" "Test User4" "$TOPIC_CHANNEL" "$OUT/$CLIENT4.txt"
joiner_client "$INVITEE" "$INVITE_CHANNEL" "$OUT/$INVITEE.txt"
joiner_client "$ALICE" "$INVITE_CHANNEL" "$OUT/$ALICE.txt"

# Kayıtlı pinger ile PING/PONG testi
{
  sleep 0.2
  printf 'PASS %s\r\n' "$PASS"
  printf 'NICK pinger\r\n'
  printf 'USER pinger 0 * :pinger\r\n'
  printf 'PING :localhost\r\n'
  sleep 0.5
} | nc localhost "$PORT" >"$OUT/ping_output.txt" 2>&1 &

# Mesajların akması için kısa bekleme
sleep 8

# Basit doğrulamalar
pass=true


if ! grep -Eq ' 001 |Welcome' "$OUT/$CLIENT1.txt"; then
  echo "[FAIL] Client1 did not receive welcome."; pass=false
else
  echo "[OK] Client1 welcome received."
fi

if ! grep -Eq ' 001 |Welcome' "$OUT/$CLIENT2.txt"; then
  echo "[FAIL] Client2 did not receive welcome."; pass=false
else
  echo "[OK] Client2 welcome received."
fi

if ! grep -q 'Client1 says hello!' "$OUT/$CLIENT2.txt"; then
  echo "[FAIL] Client2 did not receive Client1 message."; pass=false
else
  echo "[OK] Channel message delivered to Client2."
fi

if ! grep -q 'Client2 says hello!' "$OUT/$CLIENT1.txt"; then
  echo "[FAIL] Client1 did not receive Client2 message."; pass=false
else
  echo "[OK] Channel message delivered to Client1."
fi

if grep -qi 'PONG' "$OUT/ping_output.txt"; then
  echo "[OK] Server responded to PING."
else
  echo "[WARN] No PONG seen in ping_output.txt (server may require registered client)."
fi

if grep -q 'TOPIC_SET_WORKS' "$OUT/$CLIENT3.txt" && grep -q 'TOPIC_SET_WORKS' "$OUT/$CLIENT4.txt"; then
  echo "[OK] Topic change propagated to clients."
else
  echo "[FAIL] Topic change not seen by clients."; pass=false
fi

if grep -q "$INVITER $INVITEE $INVITE_CHANNEL" "$OUT/$INVITER.txt"; then
  echo "[OK] Inviter sent INVITE command."
else
  echo "[FAIL] Inviter did not send INVITE command."; pass=false
fi

if grep -q "INVITE $INVITEE :$INVITE_CHANNEL" "$OUT/$INVITEE.txt"; then
  echo "[OK] Invitee received INVITE."
else
  echo "[FAIL] Invitee did not receive INVITE."; pass=false
fi

if grep -q ' 473 ' "$OUT/$ALICE.txt"; then
  echo "[OK] Invitee was correctly blocked from joining +i channel."
else
  echo "[FAIL] Invitee was not blocked from joining +i channel."; pass=false
fi

$pass && echo "All checks passed." || { echo "Some checks failed. See $OUT/"; exit 1; }
