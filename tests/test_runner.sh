#!/bin/bash

# Simple test runner for ft_irc
# You can add more test cases as needed

echo "Starting test runner... settings PORT: $1, PASSWORD: $2"
echo "Running tests..."

# Test 1: Check if the server starts correctly
pkill -f ircserv
pkill -f telnet
echo "Starting server..."
rm -rf ./tests/server_output.txt ./tests/ping_output.txt ./tests/client1_output.txt ./tests/client2_output.txt
./ircserv $1 $2 > ./tests/server_output.txt 2>&1 &
SERVER_PID=$!
sleep 1

# Test 2: Check if the server responds to PING
echo "PING :localhost" | nc localhost $1 > ./tests/ping_output.txt &

# Test 3: Check if the client can connect
echo "Connecting client..."
telnet localhost $1 > ./tests/client1_output.txt 2>&1 &
CLIENT1_PID=$!
telnet localhost $1 > ./tests/client2_output.txt 2>&1 &
CLIENT2_PID=$!

sleep 1 && echo -e "PASS $2 \n NICK test_user\n USER test_user 0 * :Test User\n JOIN #test \n PRIVMSG #test :Client 1, World!"  > ./proc/$CLIENT1_PID/fd/0 &
sleep 1 && echo -e "PASS $2 \n NICK test_user2\n USER test_user2 1 * :Test User2\n JOIN #test \n PRIVMSG #test :Client 2, World!" > ./proc/$CLIENT2_PID/fd/0 &

# # Clean up
# kill $SERVER_PID