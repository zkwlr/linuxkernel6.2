#!/bin/bash

# Compile the module
make

# Insert the kernel module with sudo privileges
sudo insmod hw1.ko

# Display the contents of /proc/hw1
cat /proc/hw1

# Remove the kernel module with sudo privileges
sudo rmmod hw1.ko

# Clean up the compiled files
make clean
