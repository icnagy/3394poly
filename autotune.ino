int autotune_values[6] = { 0, 138, -42, 18, 0, 0};
uint16_t zero_crossings;

ISR(ANALOG_COMP_vect)
{
  if(ACSR & (1 << ACO)){
    zero_crossings++;
  }
}

void setupAutotune() {
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
}

void teardownAutotune() {
  // Turn off analog comparator interrupt
  ACSR = (0 << ACD)   |   // Analog Comparator: Enabled
         (1 << ACBG)  |   // Set ACBG to use bandgap reference for +ve input
         (0 << ACO)   |   // Analog Comparator Output: OFF
         (1 << ACI)   |   // Analog Comparator Interrupt Flag:
                          // Clear Pending Interrupt by  setting the bit
         (0 << ACIE)  |   // Analog Comparator Interrupt: Enable
         (0 << ACIC)  |   // Analog Comparator Input Capture: Disabled
         (0 << ACIS1) |   // Analog Comparator Interrupt Mode:
         (0 << ACIS0);    // Comparator Interrupt on Output Toggle
}

uint16_t getVoiceFrequency() {

  uint16_t retValue = 0;
  for(int calibration_run = 0;
      calibration_run < 2;
      calibration_run++) {

    zero_crossings = 0;

    lastMessageReceived = millis();
    while((millis() - lastMessageReceived) < 500) {
      ;
    }

    if(calibration_run == 1) { // throw away 1st(0)
      retValue = zero_crossings;
    }
  }
  return retValue;
}

void runAutotune() {

#ifdef DISPLAY_DAC_DATA
  printVoiceDacValues(&voicess[0]);
  printVoiceDacValues(&voicess[1]);
  printVoiceDacValues(&voicess[2]);
  printVoiceDacValues(&voicess[3]);
  printVoiceDacValues(&voicess[4]);
  printVoiceDacValues(&voicess[5]);
  Serial.println("------------------------------");
#endif // DISPLAY_DAC_DATA

  for(int voiceNo = 0; voiceNo < NUMBER_OF_VOICES; voiceNo++) {
    // load stored values from EEPROM
    voicess[voiceNo].dac_offset = autotune_values[voiceNo];

    // set up VCA to half way
    voicess[voiceNo].dacValues[VCA] = VCA_AUTOTUNE_VALUE;
    voicess[voiceNo].dacValues[CV] = CV_AUTOTUNE_VALUE;
    voicess[voiceNo].dacValues[MIXER] = MIXER_VCO_MAX_VALUE;
    voicess[voiceNo].frequency_at_zero_volt = getVoiceFrequency();

    int delta = voicess[0].frequency_at_zero_volt - voicess[voiceNo].frequency_at_zero_volt;

#ifdef DISPLAY_AUTOTUNE_INFO
    Serial.print("voice: ");
    Serial.print(voiceNo);
    Serial.print(" frequency: ");
    Serial.print(voicess[voiceNo].frequency_at_zero_volt);
    Serial.print(" delta: ");
    Serial.println(delta);
#endif // DISPLAY_AUTOTUNE_INFO

    if(voicess[voiceNo].frequency_at_zero_volt != 0 && abs(delta) >= 5) {

      int best_offset = 0;
      int offset_start = autotune_values[voiceNo] - (AUTOTUNE_STEP_SIZE * AUTOTUNE_WIDTH);
      int offset_end = autotune_values[voiceNo] + (AUTOTUNE_STEP_SIZE * AUTOTUNE_WIDTH);
      int offset_step = AUTOTUNE_STEP_SIZE;
      int current_frequency_delta = 0;
      uint16_t frequency_at_current_offset = 0;

      if(delta < 0) {
        offset_start = autotune_values[voiceNo] + (AUTOTUNE_STEP_SIZE * AUTOTUNE_WIDTH);
        offset_end = autotune_values[voiceNo] - (AUTOTUNE_STEP_SIZE * AUTOTUNE_WIDTH);
        offset_step = -offset_step;
      }

      for(int offset = offset_start;
              offset != offset_end;
              offset += offset_step) {

        voicess[voiceNo].dac_offset = offset;

        frequency_at_current_offset = getVoiceFrequency();                // Measure frequency

        current_frequency_delta = int(voicess[0].frequency_at_zero_volt - frequency_at_current_offset);

#ifdef DISPLAY_AUTOTUNE_INFO
        Serial.print("voice: ");
        Serial.print(voiceNo);
        Serial.print(" dac offset: ");
        Serial.print(offset);
        Serial.print(" frequency: ");
        Serial.print(frequency_at_current_offset);
        Serial.print(" freq diff: ");
        Serial.print(current_frequency_delta);
        Serial.print(" better: ");
        Serial.println(abs(current_frequency_delta) < abs(delta));
#endif // DISPLAY_AUTOTUNE_INFO

        if(abs(current_frequency_delta) < abs(delta)) {
          best_offset = offset;
          delta = current_frequency_delta;
        }
        if(abs(delta) < 3) {
          break;
        }
      }
      voicess[voiceNo].dac_offset = best_offset;

#ifdef DISPLAY_AUTOTUNE_INFO
        Serial.print("voice: ");
        Serial.print(voiceNo);
        Serial.print(" best_offset: ");
        Serial.println(best_offset);
#endif // DISPLAY_AUTOTUNE_INFO

    }
    voicess[voiceNo].dacValues[VCA] = VCA_MIN_VALUE;
  }
}
