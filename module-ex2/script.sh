#!/bin/bash

# Compile the module
make

# Insert the kernel module with sudo privileges
sudo insmod tasklet.ko

# Display the contents of /proc/hw1
cat /proc/tasklet

# Remove the kernel module with sudo privileges
sudo rmmod tasklet.ko

# Clean up the compiled files
make clean
