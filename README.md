### Compile

```
arduino-cli compile --fqbn arduino:avr:uno && \
avrdude -B 1 -V -p m328p -c arduino -P /dev/cu.usbserial-14210 -b115200 \
  -U flash:w:build/arduino.avr.uno/sketch_nov30a.ino.hex:i
```
