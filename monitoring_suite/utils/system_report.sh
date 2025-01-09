#!/bin/bash

echo "=== CPU Information ==="
lscpu | grep -E "MHz|GHz"

echo -e "\n=== Memory Speed ==="
sudo dmidecode -t memory | grep -E "Speed|Type:|Size"

echo -e "\n=== CPU Frequency Scaling ==="
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq | head -n 1

echo -e "\n=== Hardware Information ==="
sudo lshw -C memory | grep -A 5 "Memory"

echo -e "\n=== Memory Controller Info ==="
sudo lspci | grep -i memory