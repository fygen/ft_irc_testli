#!/usr/bin/bash
set -euo pipefail                                      # Hataları agresif yakala: -e (hata durdurur), -u (tanımsız değişken hata), pipefail

PORT="${1:-6667}"                                      # 1. argüman: sunucu portu (yoksa 6667)
PASS="${2:-password}"                                  # 2. argüman: sunucu parolası (yoksa 'password')
HOST="127.0.0.1"                                       # Loopback host (IPv4'e sabitledik; istersen ::1 yapabilirsin)
# ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"   # Script'in bulunduğu dizini hesapla
OUT="./tests"                                      # Tüm log/çıktıların düşeceği klasör
CHAN="#mtest"                                          # Test kanalı
mkdir -p "$OUT"                                        # Çıktı klasörünü oluştur (varsa sorun yok)

echo "Starting test runner... PORT:$PORT PASS:$PASS"    # Başlangıç mesajı

pkill -f '[./]ircserv' >/dev/null 2>&1 || true         # Önceki ircserv süreçlerini sessiz öldür (yoksa sessiz geç)
echo "Starting server..."                               # Sunucuyu başlattığımızı bildir
"./ircserv" "$PORT" "$PASS" >"$OUT/server_output.txt" 2>&1 &  # Sunucuyu arka planda başlat, logla
SERVER_PID=$!                                           # Sunucu PID'ini al

cleanup() {                                             # Çıkışta temizlik fonksiyonu
  for name in alice bob carol dave erin; do             # Tüm istemci isimleri üzerinde dön
    local pidvar="${name}_PID"                          # PID değişken adı (ör. alice_PID)
    local fifovar="${name}_IN"                          # FIFO yolu değişken adı (ör. alice_IN)
    [[ -n "${!pidvar-}" ]] && kill "${!pidvar}" >/dev/null 2>&1 || true  # İstemci nc süreçlerini öldür
    [[ -n "${!fifovar-}" ]] && rm -f "${!fifovar}" >/dev/null 2>&1 || true # FIFO'ları sil
  done
  kill "$SERVER_PID" >/dev/null 2>&1 || true            # Sunucuyu öldür
}
trap cleanup EXIT                                       # Script her nasıl biterse bitsin cleanup çalışsın

# Sunucu portu açılana kadar bekleme (maks. ~5s)
for i in {1..50}; do                                    # 50 * 0.1s = 5 saniye
  if nc -z -w 1 "$HOST" "$PORT"; then                   # Port dinliyor mu kontrolü
    echo "Server is up."                                # Dinlemeye geçtiyse bildir
    break                                               # Döngüden çık
  fi
  sleep 0.1                                             # Kısa bekleme
  if ! kill -0 "$SERVER_PID" 2>/dev/null; then         # Sunucu çöktüyse
    echo "Server crashed; logs:"                        # Logları göster
    tail -n +1 "$OUT/server_output.txt"                 # Başından itibaren sunucu logu
    exit 1                                              # Hata ile çık
  fi
  [[ $i -eq 50 ]] && { echo "Timeout waiting for $PORT"; exit 1; }  # Süre aşıldıysa hata
done

# Yardımcı: Bir dosyada pattern görünene kadar bekle (timeout ile)
wait_for() {                                            # Kullanım: wait_for <file> <regex> <seconds>
  local file="$1" pat="$2" sec="${3:-5}"                # Parametreleri al, timeout default 5s
  for _ in $(seq 1 $((sec*10))); do                     # Her 0.1s kontrol, toplam sec saniye
    grep -E "$pat" "$file" >/dev/null 2>&1 && return 0  # Pattern bulunduysa başarılı dön
    sleep 0.1                                           # 0.1s bekle
  done
  return 1                                              # Süre bitti, bulunamadı
}

# Yardımcı: Bir grep assertion (başarılı/başarısız mesajı basar ve pass=false yapar)
pass=true                                               # Genel test durumu (true→şimdilik geçti)
assert_grep() {                                         # Kullanım: assert_grep <file> <regex> "<ok>" "<fail>"
  local file="$1" pat="$2" ok="$3" fail="$4"            # Parametreler
  if grep -E "$pat" "$file" >/dev/null 2>&1; then       # Regex eşleşti mi?
    echo "[OK] $ok"                                     # Başarı mesajı
  else
    echo "[FAIL] $fail"                                 # Hata mesajı
    pass=false                                          # Genel durumu başarısız yap
  fi
}

# Kalıcı istemci oluştur (FIFO + nc). Aynı bağlantıdan tekrar tekrar komut gönderebilelim.
create_client() {                                       # Kullanım: create_client <name> <nick> <user> <outfile>
  local name="$1" nick="$2" user="$3" out="$4"          # Parametreleri al
  local fifo="$OUT/${name}.in"                          # Bu istemci için FIFO yolu
  rm -f "$fifo" && mkfifo "$fifo"                       # FIFO'yu temizle ve oluştur
  nc "$HOST" "$PORT" < "$fifo" > "$out" 2>&1 &          # nc'yi FIFO'yu stdin yaparak başlat; çıktı dosyaya
  eval "${name}_PID=$!"                                 # PID'i (ör. alice_PID) değişkene yaz
  eval "${name}_IN='$fifo'"                             # FIFO yolunu (ör. alice_IN) değişkene yaz
  eval "${name}_OUT='$out'"                             # Çıkış dosya yolunu (ör. alice_OUT)
  sleep 0.1                                             # nc hazır olsun diye kısa bekle
  send "$name" "PASS $PASS" "NICK $nick" "USER $user 0 * :$user"  # Kayıt komutlarını gönder
}

# İstemciye bir veya daha fazla IRC komutu gönder (CRLF ile)
send() {                                                # Kullanım: send <name> "CMD1" "CMD2" ...
  local name="$1"; shift                                # İlk param: isim; geri kalanı komutlar
  local pipe_var="${name}_IN"                           # FIFO değişken adını hazırla
  local pipe="${!pipe_var}"                             # FIFO gerçek yolunu indirekt al
  for cmd in "$@"; do                                   # Gönderilecek komutlarda dön
    printf '%s\r\n' "$cmd" > "$pipe"                   # Komutu CRLF ile FIFO'ya yaz
  done
}

# Basit görünürlük için kısa bekleme
short_sync() { sleep 0.2; }                             # 200ms bekleme (çıktıların dosyaya düşmesi için)

# === BAĞLANTILAR: alice (kanal kurucu), bob, carol, dave, erin ===
create_client "alice"  "alice"  "Alice"  "$OUT/alice.txt"   # alice'i bağla
create_client "bob"    "bob"    "Bob"    "$OUT/bob.txt"     # bob'u bağla
create_client "carol"  "carol"  "Carol"  "$OUT/carol.txt"   # carol'u bağla
create_client "dave"   "dave"   "Dave"   "$OUT/dave.txt"    # dave'i bağla
create_client "erin"   "erin"   "Erin"   "$OUT/erin.txt"    # erin'i bağla (limit testi için)

# Başlangıç: alice ve bob aynı kanala girsin; alice ilk giren → oper olur (ft_irc tipik davranış)
send "alice" "JOIN $CHAN"                                 # alice kanala katılır (kurucu/oper)
send "bob"   "JOIN $CHAN"                                 # bob kanala katılır
short_sync                                                     # Kısa bekleme

# Hoş geldin/NAMES gibi ilk doğrulamalar (gevşek kontrol)
wait_for "$OUT/alice.txt" 'JOIN '"$CHAN" 3 || true         # alice JOIN çıktısını bekle (yoksa geç)
wait_for "$OUT/bob.txt"   'JOIN '"$CHAN" 3 || true         # bob JOIN çıktısını bekle (yoksa geç)

# === MODE +t: Topic sadece oper set edebilsin ===
send "alice" "MODE $CHAN +t"                              # alice: +t
short_sync
assert_grep "$OUT/alice.txt" "MODE $CHAN \+t" \
  "MODE +t applied." "MODE +t not reflected to alice."
send "bob"   "TOPIC $CHAN :bob tries to set topic"        # bob (oper değil) topic değiştirmeyi dener
short_sync
assert_grep "$OUT/bob.txt" " 482 |CHANOPRIV|No..channel.operator" \
  "Non-op blocked by +t." "Non-op could set TOPIC despite +t."
send "alice" "TOPIC $CHAN :official topic"                # alice (oper) topic'i set eder
short_sync
assert_grep "$OUT/alice.txt" "TOPIC $CHAN :official topic| 332 " \
  "Op can set TOPIC under +t." "Op failed to set TOPIC under +t."

# === MODE +i: Invite-only kanal ===
send "alice" "MODE $CHAN +i"                              # alice: +i
short_sync
assert_grep "$OUT/alice.txt" "MODE $CHAN \+i" \
  "+i applied." "+i not reflected."
send "carol" "JOIN $CHAN"                                 # carol davetsiz katılmayı dener
short_sync
assert_grep "$OUT/carol.txt" " 473 |invite.?only" \
  "Invite-only blocks uninvited JOIN." "Uninvited JOIN did not fail under +i."
send "alice" "INVITE carol $CHAN"                         # carol'u davet et
short_sync
send "carol" "JOIN $CHAN"                                 # carol tekrar katılsın
short_sync
assert_grep "$OUT/carol.txt" "JOIN $CHAN| 353 | 366 " \
  "Invited user joined." "Invited user could not join with +i."

# === MODE +k: Kanal şifresi (key) ===
send "alice" "MODE $CHAN +k secret123"                    # alice: +k secret
short_sync
assert_grep "$OUT/alice.txt" "MODE $CHAN \+k" \
  "+k applied." "+k not reflected."
send "dave"  "JOIN $CHAN"                                 # dave anahtarsız denesin
short_sync
assert_grep "$OUT/dave.txt" " 475 |bad channel key|key is incorrect" \
  "JOIN blocked without key." "JOIN without key did not fail under +k."
send "dave"  "JOIN $CHAN secret123"                       # dave doğru anahtarla denesin
short_sync
assert_grep "$OUT/dave.txt" "JOIN $CHAN| 353 | 366 " \
  "JOIN with correct key succeeded." "JOIN with correct key failed."
send "alice" "MODE $CHAN -k secret123"                    # anahtarı kaldır (çoğu ft_irc -k <key> ister)
short_sync
assert_grep "$OUT/alice.txt" "MODE $CHAN -k" \
  "Key removed." "Key removal not reflected."
send "dave"  "PART $CHAN :rejoin test (no key)"           # dave ayrılsın
short_sync
send "dave"  "JOIN $CHAN"                                 # anahtarsız tekrar katılsın
short_sync
assert_grep "$OUT/dave.txt" "JOIN $CHAN| 353 | 366 " \
  "JOIN works after -k." "JOIN without key still blocked after -k."

# === MODE +l: Kullanıcı limiti ===
send "dave"  "PART $CHAN :limit test"                     # Limit senaryosu için 3 kişi kalsın (alice,bob,carol)
short_sync
send "alice" "MODE $CHAN +l 3"                            # Kanal limitini 3 yap
short_sync
assert_grep "$OUT/alice.txt" "MODE $CHAN \+l 3|MODE $CHAN \+l" \
  "+l 3 applied." "+l not reflected."
send "dave"  "JOIN $CHAN"                                 # 4. kişi olarak dave katılmayı dener
short_sync
assert_grep "$OUT/dave.txt" " 471 |channel is full" \
  "JOIN blocked by +l 3." "JOIN not blocked though channel is full."
send "alice" "MODE $CHAN -l"                              # Limit kaldır
short_sync
assert_grep "$OUT/alice.txt" "MODE $CHAN -l" \
  "Limit removed." "Limit removal not reflected."
send "dave"  "JOIN $CHAN"                                 # dave tekrar katılmalı (artık limit yok)
short_sync
assert_grep "$OUT/dave.txt" "JOIN $CHAN| 353 | 366 " \
  "JOIN works after -l." "JOIN failed after -l removal."

# === MODE +o / -o: Oper verme/alma ===
send "alice" "MODE $CHAN +o bob"                          # alice bob'a oper verir
short_sync
assert_grep "$OUT/bob.txt" "MODE $CHAN \+o bob" \
  "Bob received +o." "Bob did not receive +o."
send "bob"   "TOPIC $CHAN :bob can set topic now"         # +t açıkken bob TOPIC set etmeli
short_sync
assert_grep "$OUT/bob.txt" "TOPIC $CHAN :bob can set topic now| 332 " \
  "Op (bob) can set TOPIC under +t." "Op (bob) failed to set TOPIC under +t."
send "alice" "MODE $CHAN -o alice"                        # alice'den oper yetkisini al
short_sync
assert_grep "$OUT/alice.txt" "MODE $CHAN -o alice" \
  "Alice lost +o." "Alice -o not reflected."
send "alice" "TOPIC $CHAN :alice tries without op"        # alice artık op değil → 482 beklenir
short_sync
assert_grep "$OUT/alice.txt" " 482 |CHANOPRIV" \
  "Non-op blocked after -o." "Non-op could still set TOPIC after -o."
send "bob"   "MODE $CHAN +o alice"                        # alice'e oper geri ver
short_sync
assert_grep "$OUT/alice.txt" "MODE $CHAN \+o alice" \
  "Alice re-gained +o." "Alice +o not reflected."

# === Özet/son durum ===
$pass && { echo "All MODE tests passed."; exit 0; } \
      || { echo "Some MODE tests failed. See $OUT/ for logs."; exit 1; }   # Duruma göre çık
