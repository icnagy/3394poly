void panic() {
  for(int i=0; i < NUMBER_OF_VOICES; i++) {
    voicess[i].gate = NOTE_OFF;
    voicess[i].vca_envelope.state = ENV_IDLE;
    voicess[i].vca_envelope.value = voicess[i].vca_envelope.min_value;
    voicess[i].vcf_envelope.state = ENV_IDLE;
    voicess[i].vcf_envelope.value = voicess[i].vcf_envelope.min_value;
    voicess[i].lfo.value = 0;

    voicess[i].dacValues[CUTOFF] =      CUTOFF_ZERO_VALUE;      //   0..7000,   // Cutoff
    voicess[i].dacValues[RESONANCE] =   RESONANCE_MIN_VALUE;    //   0..2500,   // Resonance
    voicess[i].dacValues[MIXER] =       MIXER_ZERO_VALUE;       //   0..4000,   // Mixer Balance
    voicess[i].dacValues[VCA] =         VCA_MIN_VALUE;          //   0..4200    // VCA
    voicess[i].dacValues[PWM] =         PWM_ZERO_VALUE;         //   0..2300,   // PWM
    voicess[i].dacValues[CV] =          CV_ZERO_VALUE;          //   0..8192,   // Key CV
    voicess[i].dacValues[WAVE_SELECT] = WAVE_SELECT_SQR;        //   0..4500,   // Wave Select
    voicess[i].dacValues[MOD_AMT] =     MOD_AMT_MIN_VALUE;      //   0..4000,   // Mod Amount
#ifdef DISABLE_AUTOTUNE
    voicess[i].dac_offset = autotune_values[i];
#endif // DISABLE_AUTOTUNE
  }
}

void updatePitch(voice* voice) {

  uint16_t pitch = voice->dacValues[CV]  + voice->dac_offset;
  uint16_t gain =
             pitch > CV_GAIN_SWITCH_POINT ? CV_GAIN_HIGH : CV_GAIN_LOW;
  uint16_t offset =
             pitch > CV_GAIN_SWITCH_POINT ? (pitch - CV_GAIN_SWITCH_POINT) : pitch;

  // Serial.print("CV: ");Serial.print(pitch);

  updateDAC(gain, constrain(offset,
                            CV_MIN_VALUE,
                            CV_MAX_VALUE));
}

void updateModAmount(voice* voice) {
  // Serial.print("|MOD: ");Serial.print(modAmount);
  uint16_t modAmount = voice->dacValues[MOD_AMT];

  updateDAC(MOD_AMT_GAIN, constrain(modAmount,
                                    MOD_AMT_MIN_VALUE,
                                    MOD_AMT_MAX_VALUE));
}

void updateWaveSelect(voice* voice) {
  // Serial.print("|WS: ");Serial.print(waveSelect);
  uint16_t waveSelect = voice->dacValues[WAVE_SELECT];
  updateDAC(WAVE_SELECT_GAIN, constrain(waveSelect,
                                        WAVE_SELECT_MIN_VALUE,
                                        WAVE_SELECT_MAX_VALUE));
}

void updatePwm(voice* voice) {
  // Serial.print("|PWM: ");Serial.print(pwm);
  uint16_t pwm = voice->dacValues[PWM];
  updateDAC(PWM_GAIN, constrain(pwm,
                                PWM_MIN_VALUE,
                                PWM_MAX_VALUE));
}

void updateMixer(voice* voice) {
  // Serial.print("|MIX: ");Serial.print(mixer);
  uint16_t mixer = voice->dacValues[MIXER];
  updateDAC(MIXER_GAIN, constrain(mixer,
                                  MIXER_MIN_VALUE,
                                  MIXER_MAX_VALUE));
}

void updateResonance(voice *voice) {
  // Serial.print("|RES: ");Serial.print(resonance);
  uint16_t resonance = voice->dacValues[RESONANCE];
  updateDAC(RESONANCE_GAIN, constrain(resonance,
                                      RESONANCE_MIN_VALUE,
                                      RESONANCE_MAX_VALUE));
}

void updateCutoff(voice *voice) {
  // Serial.print("|VCF: ");Serial.print(cutoff);
  uint16_t cutoff = voice->dacValues[CUTOFF];
  if(cutoff > CUTOFF_GAIN_THRESHOLD) {
    updateDAC(CUTOFF_GAIN_HIGH, constrain((cutoff - CUTOFF_GAIN_THRESHOLD),
                                          CUTOFF_MIN_VALUE,
                                          CUTOFF_MAX_VALUE - CUTOFF_GAIN_THRESHOLD));
  } else {
    updateDAC(CUTOFF_GAIN_LOW, constrain(cutoff,
                                         CUTOFF_MIN_VALUE,
                                         CUTOFF_GAIN_THRESHOLD));
  }
}

void updateVca(voice *voice) {
  // Serial.print("|VCA: ");Serial.print(vca);
  uint16_t vca = voice->dacValues[VCA];
  updateDAC(VCA_GAIN, constrain(vca,
                                VCA_MIN_VALUE,
                                VCA_MAX_VALUE));
  // Serial.println();
}

void updateDAC(uint16_t gain, uint16_t offset) {
  uint16_t command = k_writeChannelA | (gain & 0x0FFF);

  // Serial.print("|G: ");Serial.print(gain);
  // Serial.print("|O: ");Serial.print(offset);

  m_outputChipSelectDAC = LOW;     // Disable DAC latch

  SPI.transfer(command >> 8);
  SPI.transfer(command & 0xFF);

  m_outputChipSelectDAC = HIGH;    // DAC latch Vout

  command = k_writeChannelB | ((offset >> 1) & 0x0FFF);

  m_outputChipSelectDAC = LOW;     // Disable DAC latch

  SPI.transfer(command >> 8);
  SPI.transfer(command & 0xFF);

  m_outputChipSelectDAC = HIGH;    // DAC latch Vout

  // Delay a bit to allow DAC to settle
  // DAC has 4.5uS settling time
  // CPU runs at 16MHz
  // 0.0000045/(1/16000000) = 72+ cycles to wait
  // Serial.println("foo");
  __builtin_avr_delay_cycles(72);
}
