### Compile

```
arduino-cli compile -v --fqbn arduino:avr:uno && arduino-cli upload -v -p /dev/$(ls /dev | grep cu.usb) --fqbn arduino:avr:uno && screen /dev/$(ls /dev | grep tty.usb) 115200
```

### Doodle

12 bit DAC

-0.75V / Octave => -0.75 V / 12 semitones = -0.0625 V per semitone

0V = F0 = 500Hz

With DAC Vref = 5V

1 bit = 5V / 4096 = 0.001220703125 V

(Voltage unit per octave / number of semitones) / ( DAC Voltage reference / DAC bits available) = bits available per semitone

(0.75 / 12) / (5 / 4096) = 51.2 bits available for tuning / semitone


With DAC Vref = 4V

1 bit = 4V / 4096 = 0.0009765625 V

(Voltage unit per octave / number of semitones) / ( DAC Voltage reference / DAC bits available) = bits available per semitone

(0.75 / 12) / (4 / 4096) = 64 bits available for tuning / semitone

LM337

Vout = Vin * (1 + R2/R1)
-1.25 * (1+510/120)

### DAC

Example 6-4: Bipolar Voltage Source With Selectable Gain and Offset

Vref = 5V

R1, R2, R3: 10k

Gain: DACa digital input
Offset: DACb digital input

Gain | Offset | Voltage |
-----|--------|---------|
0    | 0      |  0.0
0    | 500    |  0.57
0    | 1000   |  1.15
0    | 1500   |  1.72
0    | 2000   |  2.29
0    | 2500   |  2.86
0    | 3000   |  3.43
0    | 3500   |  4.01
0    | 4000   |  4.58
500  | 0      | -0.54
500  | 500    |  0.0
500  | 1000   |  0.58
500  | 1500   |  1.15
500  | 2000   |  1.72
500  | 2500   |  2.29
500  | 3000   |  2.87
500  | 3500   |  3.44
500  | 4000   |  4.02
1000 | 0      | -1.12
1000 | 500    | -0.54
1000 | 1000   |  0.0
1000 | 1500   |  0.58
1000 | 2000   |  1.15
1000 | 2500   |  1.72
1000 | 3000   |  2.29
1000 | 3500   |  2.87
1000 | 4000   |  3.44
1500 | 0      | -1.68
1500 | 500    | -1.11
1500 | 1000   | -0.54
1500 | 1500   |  0.0
1500 | 2000   |  0.58
1500 | 2500   |  1.15
1500 | 3000   |  1.72
1500 | 3500   |  2.30
1500 | 4000   |  2.88
2000 | 0      | -2.25
2000 | 500    | -1.69
2000 | 1000   | -1.11
2000 | 1500   | -0.54
2000 | 2000   |  0.0
2000 | 2500   |  0.58
2000 | 3000   |  1.15
2000 | 3500   |  1.73
2000 | 4000   |  2.30
2500 | 0      | -2.83
2500 | 500    | -2.26
2500 | 1000   | -1.68
2500 | 1500   | -1.11
2500 | 2000   | -0.54
2500 | 2500   |  0.0
2500 | 3000   |  0.57
2500 | 3500   |  1.15
2500 | 4000   |  1.72
3000 | 0      | -3.39
3000 | 500    | -2.82
3000 | 1000   | -2.25
3000 | 1500   | -1.68
3000 | 2000   | -1.11
3000 | 2500   | -0.54
3000 | 3000   |  0.0
3000 | 3500   |  0.58
3000 | 4000   |  1.15
3500 | 0      | -3.96
3500 | 500    | -3.93
3500 | 1000   | -2.82
3500 | 1500   | -2.25
3500 | 2000   | -1.68
3500 | 2500   | -1.11
3500 | 3000   | -0.54
3500 | 3500   |  0.0
3500 | 4000   |  0.58
4000 | 0      | -4.52
4000 | 500    | -3.96
4000 | 1000   | -3.38
4000 | 1500   | -2.82
4000 | 2000   | -2.25
4000 | 2500   | -1.68
4000 | 3000   | -1.11
4000 | 3500   | -0.54
4000 | 4000   |  0.0

## AS3394

// Voice:
// {
//   0..7000,   // Key CV
//   0..3500,   // Mod Amount
//   0..4000,   // Wave Select
//   0..2000,   // PWM
//   0..3500,   // Mixer Balance
//   0..2000,   // Resonance
//   0..6000,   // Cutoff
//   0..3500    // VCA
// }
