#!/usr/bin/env bash

set -e

cmake .
make MyBot
./halite -d "240 160" "./MyBot" "./old_bots/v2/MyBot"
