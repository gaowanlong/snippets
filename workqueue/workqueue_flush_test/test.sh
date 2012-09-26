#/bin/bash

make
sync
sync
echo testing...
trap 'echo interrupt test' INT
sudo insmod wflush.ko
sleep 60000
sudo rmmod wflush.ko
echo test done

