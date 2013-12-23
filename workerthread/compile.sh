#!/bin/sh
g++ -O2 -std=c++11 -c main.cpp
g++ -std=c++11 -o main.o -L/usr/lib/x86_64-linux-gnu -lboost_filesystem -lboost_system -lboost_thread -pthread -Wl,--no-as-needed
