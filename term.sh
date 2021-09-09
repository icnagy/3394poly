#!/bin/bash

select port in $(ls /dev/cu.usb*)
do
  screen $port 115200
  break
done
reset
