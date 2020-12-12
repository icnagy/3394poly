### Compile

```
arduino-cli compile -v --fqbn arduino:avr:uno && arduino-cli upload -v -p /dev/$(ls /dev | grep cu.usb) --fqbn arduino:avr:uno
```
