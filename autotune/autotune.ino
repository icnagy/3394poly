
uint8_t old_didr1;
uint8_t old_adcsra;
uint8_t old_adcsrb;
uint8_t old_acsr;
uint8_t old_admux;

uint16_t zero_crossings;

void setup () {
  cli();

  old_didr1 = DIDR1;
  old_adcsra = ADCSRA;
  old_adcsrb = ADCSRB;
  old_admux = ADMUX;
  old_acsr = ACSR;
  DIDR1 |= (1 << AIN0D) | // Disable Digital Inputs at AIN0 and AIN1
           (1 << AIN1D);

  ADCSRA &= ~(1 << ADEN); // Disable the ADC

  ADCSRB |= (1 << ACME);  // Set ACME bit in ADCSRB to use external analog input
                          // at AIN1 -ve input

  ADMUX = (1 << REFS1) |
          (1 << REFS0) |
          0x4;            // select A4 as input

  ACSR = (0 << ACD)   |   // Analog Comparator: Enabled
         (1 << ACBG)  |   // Set ACBG to use bandgap reference for +ve input
         (0 << ACO)   |   // Analog Comparator Output: OFF
         (1 << ACI)   |   // Analog Comparator Interrupt Flag:
                          // Clear Pending Interrupt by  setting the bit
         (1 << ACIE)  |   // Analog Comparator Interrupt: Enable
         (0 << ACIC)  |   // Analog Comparator Input Capture: Disabled
         (0 << ACIS1) |   // Analog Comparator Interrupt Mode:
         (0 << ACIS0);    // Comparator Interrupt on Output Toggle
  sei();

  Serial.begin(9600);
  Serial.println("Hello");
  // Serial.println(old_didr1);    // 0   0x00 0000 0000
  // Serial.println(old_adcsra);   // 135 0x87 1000 0111
  // Serial.println(old_adcsrb);   // 0   0x00 0000 0000
  // Serial.println(old_admux);    // 0   0x00 0000 0000
  // Serial.println(old_acsr);     // 48  0x30 0000 0011

  // Serial.println(DIDR1);        // 3   0x03 0000 0011 OK
  // Serial.println(ADCSRA);       // 7   0x07 0000 0111 OK
  // Serial.println(ADCSRB);       // 64  0x40 0100 0000 OK
  // Serial.println(ADMUX);        // 196 0xC4 1100 0100 OK
  // Serial.println(ACSR);         // 104 0x68 0110 1000 ?
                                //            ?
  Serial.println("done");
}

uint32_t lastMessageReceived = 0;

void loop() {
  // let's measure each voice at 0V and assume that the error is linear across 8V
  //
  for(int voiceNo = 0; voiceNo < 6; voiceNo++){
    // set up voice (i / 3) VCA to half way
    for(int calibration_run = 0; calibration_run < 2; calibration_run++) {
      zero_crossings = 0;
      lastMessageReceived = millis();
      while((millis() - lastMessageReceived) < 1000) {
        ;
      }
      if(calibration_run == 1) { // throw away 1st(0) and 3rd(2) measurement
        Serial.print("calibration_run:");
        Serial.print(calibration_run);
        Serial.print(" zero_crossings:");
        Serial.println(zero_crossings);
        // voicess[voiceNo].error = zero_crossings
      // }
    }
  }

  // Simple frequency counting:
  // if(millis() - lastMessageReceived > 1000) {
  //   lastMessageReceived = millis();
  //   Serial.println(zero_crossings);
  //   zero_crossings = 0;
  // }
}

ISR(ANALOG_COMP_vect)
{
  if(ACSR & (1 << ACO)){
    zero_crossings++;
  }
}
