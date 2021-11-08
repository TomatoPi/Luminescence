#!/usr/bin/bash

cd ~/Documents/2.0/progs/Optopoulpe/jackproxy

# xfce4-terminal -T "Controller"      -e "cat pipes/apc2ctrl |  ./controller save.txt pipes/ard2ctrl pipes/ctrl2ard 1> pipes/ctrl2apc"  &
# xfce4-terminal -T "Arduino-Bridge"  -e "cat pipes/ctrl2ard |  ./arduino-bridge /dev/ttyACM0                       1> pipes/ard2ctrl"  &
# xfce4-terminal -T "APC40-Mapper"    -e "cat pipes/ctrl2apc |  ./apc40-mapping pipes/j2apc pipes/apc2j             1> pipes/apc2ctrl"  &
# xfce4-terminal -T "Ctrl-2-APC40"    -e "cat pipes/apc2j    |  ./jackproxy-from-stdin"                                                 &
# xfce4-terminal -T "APC40-2-Ctrl"    -e "                      ./jackproxy-to-stdout                               1> pipes/j2apc"     &

xfce4-terminal -T "Controller"      --geometry 80x10
xfce4-terminal -T "Arduino-Bridge"  --geometry 80x10
xfce4-terminal -T "APC40-Mapper"    --geometry 80x10
xfce4-terminal -T "Ctrl-2-APC40"    --geometry 80x10
xfce4-terminal -T "APC40-2-Ctrl"    --geometry 80x10