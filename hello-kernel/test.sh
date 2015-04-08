#!/bin/bash
# Tests the kernel module
echo 'Begin tests'

# Build
echo 'Running "make all" to build module'
make all

# Load module
echo 'Running "sudo insmod hello.ko" to load module'
sudo insmod hello.ko

# Test if module loaded
echo 'Running "lsmod | grep 'hello'" to test if module loaded properly'
lsmod | grep 'hello'

# Unload module
echo 'Running "sudo rmmod hello" to unload module'
sudo rmmod hello

# Test if module unloaded
echo 'Running "lsmod | grep 'hello'" to test if module unloaded properly'
lsmod | grep 'hello'

# Clean up like a good kid
echo 'Running "make clean" to clean everything up'
make clean

# Check module
echo 'Running "tail -n 10 /var/log/syslog" to see if module did what it was supposed to'
tail -n 10 /var/log/syslog
journalctl -r

echo 'Tests finished'
