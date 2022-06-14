#!/bin/bash
arduino-cli compile --clean -v --fqbn arduino:avr:uno --build-property "build.extra_flags=-g2 -Og" --output-dir ./build

select port in $(ls /dev/cu.usb*)
do
  echo "you have chosen $port"
  arduino-cli upload -v -p $port --fqbn arduino:avr:uno --input-dir ./build
  # arduino-cli monitor -p $port -c "baudrate=115200"
  #&& screen $port 38400
  break
done
