#!/bin/bash

select port in $(ls /dev/cu.usb*)
do
  echo "you have chosen $port"
  arduino-cli compile -v --fqbn arduino:avr:uno && arduino-cli upload -v -p $port --fqbn arduino:avr:uno && screen $port 115200
  break
done

