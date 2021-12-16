#!/bin/bash

arduino-cli compile -v --fqbn arduino:avr:uno

select port in $(ls /dev/cu.usb*)
do
  echo "you have chosen $port"
  arduino-cli upload -v -p $port --fqbn arduino:avr:uno
  #&& screen $port 38400
  break
done
