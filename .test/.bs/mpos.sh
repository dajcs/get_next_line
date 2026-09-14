#!/usr/bin/env bash

while true; do
    eval "$(xdotool getmouselocation --shell)"
    printf "\rX=%-5s Y=%-5s" "$X" "$Y"
    sleep 0.05
done

