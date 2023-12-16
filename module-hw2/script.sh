#!/bin/bash

# Compile the module
make

# Insert the kernel module with sudo privileges
sudo insmod hw2.ko

# Display the contents of /proc/hw1
cat /proc/hw2

# Remove the kernel module with sudo privileges
sudo rmmod hw2.ko

# Clean up the compiled files
make clean
