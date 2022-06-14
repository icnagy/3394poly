#define NUMBER_OF_VOICES 6

#define CUTOFF_MIN_VALUE      0     // The smallest DAC value for this CV
#define CUTOFF_MAX_VALUE      6985  // The largest DAC value for this CV
#define CUTOFF_ZERO_VALUE     500   // Where to reset this CV value

#define VCA_MIN_VALUE  0            // The smallest DAC value for this CV
#define VCA_MAX_VALUE  4064         // The largest DAC value for this CV

#define NOTE_OFF 0
#define NOTE_ON  1

#define CC_FILTERATTACK     15
#define CC_FILTERDECAY      16
#define CC_FILTERSUSTAIN    17
#define CC_FILTERRELEASE    18

#define CC_AMPATTACK        23
#define CC_AMPDECAY         24
#define CC_AMPSUSTAIN       25
#define CC_AMPRELEASE       26

enum LFO_SHAPE {
  LFO_SHAPE_SAW,
  LFO_SHAPE_RMP,
  LFO_SHAPE_TRI,
  LFO_SHAPE_SQR,
  LFO_SHAPE_LAST
};

#define LFO_SAW         0
#define LFO_RMP         1
#define LFO_TRI         2
#define LFO_SQR         3
#define LFO_PHASE_UP    0
#define LFO_PHASE_DOWN  1
#define LFO_MIN_VALUE   0
#define LFO_MAX_VALUE   1024

typedef struct lfo_structure {
  uint8_t shape;
  uint8_t phase;
  uint16_t rate;
  uint16_t min_value;
  uint16_t max_value;
  uint16_t accumulator;
  int16_t value;
} lfo;

#define ENV_IDLE    0
#define ENV_ATTACK  1
#define ENV_DECAY   2
#define ENV_SUSTAIN 3
#define ENV_RELEASE 4

typedef struct envelope_structure {
  uint8_t state;
  uint16_t min_value;
  uint16_t max_value;
  uint32_t accumulator;
  uint16_t value;
  uint16_t attack_rate;
  uint16_t decay_rate;
  uint16_t sustain_value;
  uint16_t release_rate;
} envelope;

typedef struct voice_structure {
  envelope_structure vca_envelope = {
    ENV_IDLE,          // state
    VCA_MIN_VALUE,     // min_value
    VCA_MAX_VALUE,     // max_value
    VCA_MIN_VALUE,     // accumulator
    VCA_MIN_VALUE,     // value 0 - 3500
    0,                 // attack_rate
    0,                 // decay_rate
    0,                 // sustain_value
    0                  // release_rate
  };
  envelope_structure vcf_envelope = {
    ENV_IDLE,
    CUTOFF_MIN_VALUE,  // min_value
    CUTOFF_MAX_VALUE,  // max_value
    CUTOFF_MIN_VALUE,  // accumulator
    CUTOFF_MIN_VALUE,  // value 0 - 6000
    0,                 // attack_rate
    0,                 // decay_rate
    0,                 // sustain_value
    0                  // release_rate
  };
  uint8_t gate = 0;
  uint8_t note = 0;
  uint8_t velocity = 0;
} voice;

voice voicess[6];
uint8_t controlValues[32];

uint16_t exp_vcf_lookup[128] = {
  4989, 2494, 1663, 1247, 997, 831, 712, 623, 554, 498, 453, 415, 383, 356, 332,
  311, 293, 277, 262, 249, 237, 226, 216, 207, 199, 191, 184, 178, 172, 166, 160,
  155, 151, 146, 142, 138, 134, 131, 127, 124, 121, 118, 116, 113, 110, 108, 106,
  103, 101, 99, 97, 95, 94, 92, 90, 89, 87, 86, 84, 83, 81, 80, 79, 77, 76, 75,
  74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 63, 62, 61, 60, 60, 59, 58, 58,
  57, 56, 56, 55, 54, 54, 53, 53, 52, 51, 51, 50, 50, 49, 49, 48, 48, 47, 47, 47,
  46, 46, 45, 45, 44, 44, 44, 43, 43, 43, 42, 42, 41, 41, 41, 40, 40, 40, 39, 39,
  39, 38
};

uint16_t exp_vca_lookup[128] = {
  2903, 1451, 967, 725, 580, 483, 414, 362, 322, 290, 263, 241, 223, 207, 193,
  181, 170, 161, 152, 145, 138, 131, 126, 120, 116, 111, 107, 103, 100, 96, 93,
  90, 87, 85, 82, 80, 78, 76, 74, 72, 70, 69, 67, 65, 64, 63, 61, 60, 59, 58, 56,
  55, 54, 53, 52, 51, 50, 50, 49, 48, 47, 46, 46, 45, 44, 43, 43, 42, 42, 41, 40,
  40, 39, 39, 38, 38, 37, 37, 36, 36, 35, 35, 34, 34, 34, 33, 33, 32, 32, 32, 31,
  31, 31, 30, 30, 30, 29, 29, 29, 29, 28, 28, 28, 27, 27, 27, 27, 26, 26, 26, 26,
  25, 25, 25, 25, 25, 24, 24, 24, 24, 23, 23, 23, 23, 23, 23, 22, 22
};

void printEnvelopeParams(voice *voice) {
  Serial.print(" VCAVALUE: ");Serial.print(voice->vca_envelope.value);
  // Serial.print(" VCAACC: ");Serial.print(voice->vca_envelope.accumulator);
  Serial.print(" VCASTATE: ");Serial.print(voice->vca_envelope.state*1000);
  // Serial.print(" AEA: ");Serial.print(voice->vca_envelope.attack_rate);
  // Serial.print(" AED: ");Serial.print(voice->vca_envelope.decay_rate);
  // Serial.print(" AES: ");Serial.print(voice->vca_envelope.sustain_value);
  // Serial.print(" AER: ");Serial.print(voice->vca_envelope.release_rate);
  // Serial.print(" MIN: ");Serial.print(voice->vca_envelope.min_value);
  // Serial.print(" MAX: ");Serial.print(voice->vca_envelope.max_value);
  // Serial.print(" ACCA: ");Serial.print(controlValues[CC_AMPATTACK]);
  // Serial.print(" ACCD: ");Serial.print(controlValues[CC_AMPDECAY]);
  // Serial.print(" ACCS: ");Serial.print(controlValues[CC_AMPSUSTAIN]);
  // Serial.print(" ACCR: ");Serial.print(controlValues[CC_AMPRELEASE]);

  Serial.print(" VCFVALUE: ");Serial.print(voice->vcf_envelope.value);
  // Serial.print(" VCFACC: ");Serial.print(voice->vcf_envelope.accumulator);
  Serial.print(" VCFSTATE: ");Serial.print(voice->vcf_envelope.state*1000);
  // Serial.print(" FEA: ");Serial.print(voice->vcf_envelope.attack_rate);
  // Serial.print(" FED: ");Serial.print(voice->vcf_envelope.decay_rate);
  // Serial.print(" FES: ");Serial.print(voice->vcf_envelope.sustain_value);
  // Serial.print(" FER: ");Serial.print(voice->vcf_envelope.release_rate);
  // Serial.print(" MIN: ");Serial.print(voice->vcf_envelope.min_value);
  // Serial.print(" MAX: ");Serial.print(voice->vcf_envelope.max_value);
  // Serial.print(" FCCA: ");Serial.print(controlValues[CC_FILTERATTACK]);
  // Serial.print(" FCCD: ");Serial.print(controlValues[CC_FILTERDECAY]);
  // Serial.print(" FCCS: ");Serial.print(controlValues[CC_FILTERSUSTAIN]);
  // Serial.print(" FCCR: ");Serial.print(controlValues[CC_FILTERRELEASE]);
  Serial.println();
}

void updateEnvelope(envelope_structure *envelope, voice *voice) {
  // Check gate
  switch(envelope->state) {
    case ENV_IDLE:
      // Serial.println("IDLE");
      if(voice->gate) { // note on
        envelope->state = ENV_ATTACK;
        envelope->accumulator = envelope->min_value;
        envelope->value = envelope->min_value;
      }
      break;
    case ENV_ATTACK:
      // Serial.println("ATTACK");
      if(voice->gate) {

        // Serial.print("ATTACK");
        // Serial.print(" envelope->value ");Serial.print(envelope->value);
        // Serial.print(" envelope->attack_rate ");Serial.print(envelope->attack_rate);
        // Serial.print(" DIFF ");Serial.print((envelope->accumulator + (long)envelope->attack_rate));
        // Serial.print(" COND ");Serial.println((envelope->accumulator + (long)envelope->attack_rate) >= ((long)envelope->max_value << 4));

        if((envelope->accumulator + (long)envelope->attack_rate) >= ((long)envelope->max_value << 4)) {
          envelope->accumulator = ((long)envelope->max_value << 4);
          envelope->value = envelope->max_value;
          envelope->state = ENV_DECAY;
        } else {
          envelope->accumulator += envelope->attack_rate;
          envelope->value = (envelope->accumulator >> 4);
        }

      }
      break;
    case ENV_DECAY:
      // Serial.print("DECAY");
      // Serial.print(" envelope->value ");Serial.print(envelope->value);
      // Serial.print(" envelope->decay_rate ");Serial.print(envelope->decay_rate);
      // Serial.print(" DIFF ");Serial.print((envelope->accumulator - (long)envelope->decay_rate));
      // Serial.print(" SUST ");Serial.print(((long)envelope->sustain_value << 4));
      // Serial.print(" COND ");Serial.println((envelope->accumulator - (long)envelope->decay_rate) <= ((long)envelope->sustain_value << 4));
      if(voice->gate) {

        if((envelope->accumulator - (long)envelope->decay_rate) <= ((long)envelope->sustain_value << 4)) {
          // reached sustain level and gate is still on
          envelope->state = ENV_SUSTAIN;
          // envelope->accumulator = envelope->sustain_value << 4;
          // envelope->value = envelope->sustain_value;
        } else {
          envelope->accumulator -= envelope->decay_rate;
          envelope->value = envelope->accumulator >> 4;
        }

      } else {
        // This case should not happen, because note off will set state to release
      }
      break;
    case ENV_SUSTAIN:
      // Serial.println("SUSTAIN");
      if(!voice->gate) {
        envelope->state = ENV_RELEASE;
      }
      // if(voice->gate) {
      //   envelope->value = envelope->sustain_value;
      // } else {
      //   envelope->state = ENV_RELEASE;
      // }
      break;
    case ENV_RELEASE:
      // Serial.print("RELEASE");
      // Serial.print(" envelope->value ");Serial.print(envelope->value);
      // Serial.print(" envelope->release_rate ");Serial.print(envelope->release_rate);
      // Serial.print(" ACC ");Serial.print(envelope->accumulator);
      // Serial.print(" COND ");Serial.println(envelope->accumulator <= (long)envelope->release_rate);

      if(envelope->accumulator <= (long)envelope->release_rate) {
        envelope->accumulator = envelope->min_value;
        envelope->value = envelope->min_value;
        envelope->state = ENV_IDLE;
      } else {
        envelope->accumulator -= envelope->release_rate;
        envelope->value = envelope->accumulator >> 4;
      }

      break;
    default:
      envelope->accumulator = envelope->min_value;
      envelope->value = envelope->min_value;
  }
}

uint32_t lastMessageReceived = 0;

void setup() {

  // Serial.begin(115200);
  Serial.begin(9600); // MIDI?

  lastMessageReceived = millis();
  printEnvelopeParams(&voicess[0]);

  controlValues[CC_AMPATTACK] = 2;
  controlValues[CC_AMPDECAY] = 15;
  controlValues[CC_AMPSUSTAIN] = 45;
  controlValues[CC_AMPRELEASE] = 4;

  controlValues[CC_FILTERATTACK] = 2;
  controlValues[CC_FILTERDECAY] = 15;
  controlValues[CC_FILTERSUSTAIN] = 15;
  controlValues[CC_FILTERRELEASE] = 2;
}

bool brunEnvelopes = false;

void loop() {

  if (Serial.available() > 0) {    // Midi/Serial input available?
    char rx_byte = 0;
    rx_byte = Serial.read();       // Get midi/serial input

    // Serial.print(rx_byte);
    processSerialInput(rx_byte);
  }

  if(millis() - lastMessageReceived > 25) {
    lastMessageReceived = millis();

    if(brunEnvelopes) {
      voicess[0].vca_envelope.attack_rate = exp_vca_lookup[controlValues[CC_AMPATTACK]];
      voicess[0].vca_envelope.decay_rate = exp_vca_lookup[controlValues[CC_AMPDECAY]];
      // voicess[0].vca_envelope.sustain_value = (controlValues[CC_AMPSUSTAIN] << 4) + (controlValues[CC_AMPSUSTAIN] << 3) + (controlValues[CC_AMPSUSTAIN] << 1) + controlValues[CC_AMPSUSTAIN];
      voicess[0].vca_envelope.sustain_value = controlValues[CC_AMPSUSTAIN] * 32;
      voicess[0].vca_envelope.release_rate = exp_vca_lookup[controlValues[CC_AMPRELEASE]];

      voicess[0].vcf_envelope.attack_rate = exp_vcf_lookup[controlValues[CC_FILTERATTACK]];
      voicess[0].vcf_envelope.decay_rate = exp_vcf_lookup[controlValues[CC_FILTERDECAY]];
      // voicess[0].vcf_envelope.sustain_value = (controlValues[CC_FILTERSUSTAIN] << 5) + (controlValues[CC_FILTERSUSTAIN] << 4) - controlValues[CC_FILTERSUSTAIN];
      voicess[0].vcf_envelope.sustain_value = controlValues[CC_FILTERSUSTAIN] * 55;
      voicess[0].vcf_envelope.release_rate = exp_vcf_lookup[controlValues[CC_FILTERRELEASE]];

      updateEnvelope(&voicess[0].vca_envelope, &voicess[0]);
      updateEnvelope(&voicess[0].vcf_envelope, &voicess[0]);
      printEnvelopeParams(&voicess[0]);
    }
  //   printEnvelopeParams(&voicess[0]);
  //   printEnvelopeParams(&voicess[1]);
  //   printEnvelopeParams(&voicess[2]);
  //   printEnvelopeParams(&voicess[3]);
  //   Serial.println("------------------------------");
  }
}

void voiceNoteOn(int voiceNo, uint8_t note, uint8_t velocity) {
  voicess[voiceNo].gate = NOTE_ON;
  voicess[voiceNo].note = note;
  voicess[voiceNo].vca_envelope.state = ENV_ATTACK;
  voicess[voiceNo].vcf_envelope.state = ENV_ATTACK;
}

void voiceNoteOff(int voiceNo, uint8_t note, uint8_t velocity) {
  voicess[voiceNo].gate = NOTE_OFF;
  voicess[voiceNo].note = note;
  voicess[voiceNo].velocity = velocity;
  voicess[voiceNo].vca_envelope.state = ENV_RELEASE;
  voicess[voiceNo].vcf_envelope.state = ENV_RELEASE;
}

uint8_t selectedVoiceNumber = 0;
voice *selectedVoice;

void processSerialInput(char rx_byte) {
  // Serial.println((int)(rx_byte - 49));
  switch(rx_byte) {
    case 49:  // 1
    case 50:  // 2
    case 51:  // 3
    case 52:  // 4
    case 53:  // 5
    case 54: {// 6
      uint8_t note = 49;
      selectedVoiceNumber = rx_byte - 49;
      selectedVoice = &voicess[rx_byte - 49];
      selectedVoice->gate == NOTE_ON ? voiceNoteOff(selectedVoiceNumber, note, 80) : voiceNoteOn(selectedVoiceNumber, note, 80);
    }
    break;
    case 81: { // Q
      controlValues[CC_AMPATTACK]++;
    }
    break;
    case 65: { // A
      controlValues[CC_AMPATTACK]--;
    }
    break;
    case 87: { // W
      controlValues[CC_AMPDECAY]++;
    }
    break;
    case 83: { // S
      controlValues[CC_AMPDECAY]--;
    }
    break;
    case 69: { // E
      controlValues[CC_AMPSUSTAIN]++;
    }
    break;
    case 68: { // D
      controlValues[CC_AMPSUSTAIN]--;
    }
    break;
    case 82: { // R
      controlValues[CC_AMPRELEASE]++;
    }
    break;
    case 70: { // F
      controlValues[CC_AMPRELEASE]--;
    }
    break;
    case 84: { // T
      controlValues[CC_FILTERATTACK]++;
    }
    break;
    case 71: { // G
      controlValues[CC_FILTERATTACK]--;
    }
    break;
    case 89: { // Y
      controlValues[CC_FILTERDECAY]++;
    }
    break;
    case 72: { // H
      controlValues[CC_FILTERDECAY]--;
    }
    break;
    case 85: { // U
      controlValues[CC_FILTERSUSTAIN]++;
    }
    break;
    case 74: { // J
      controlValues[CC_FILTERSUSTAIN]--;
    }
    break;
    case 73: { // I
      controlValues[CC_FILTERRELEASE]++;
    }
    break;
    case 75: { // K
      controlValues[CC_FILTERRELEASE]--;
    }
    break;
    case 122: { // z
      brunEnvelopes = !brunEnvelopes;
    }
  }

  // Serial.print("1/2/3");
  // Serial.print("  CV: q/a");
  // Serial.print("  MOD: w/s");
  // Serial.print("  WAV: e/d");
  // Serial.print("  PWM: r/f");
  // Serial.print("  MIX: t/g");
  // Serial.print("  VCF: y/h");
  // Serial.print("  RES: u/j");
  // Serial.print("  VCA: i/k");
  // Serial.println("  OFS: o/l");
}
