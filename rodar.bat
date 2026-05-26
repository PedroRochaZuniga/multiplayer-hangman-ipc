@echo off

gcc server.c -o server -lws2_32
gcc client.c -o client -lws2_32

start cmd /k server.exe
start cmd /k client.exe
start cmd /k client.exe