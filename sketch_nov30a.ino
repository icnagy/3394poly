#include <SPI.h>
#include <DirectIO.h>

#define VREF 5
#define DAC_STEPS 4096
#define VOLT_PER_OCTAVE_NOTE 0.75/12
#define DAC_STEP_PER_NOTE (VOLT_PER_OCTAVE_NOTE) / (VREF / DAC_STEPS)

#define CV          0
#define CV_GAIN_LOW  3500
#define CV_GAIN_HIGH 0
#define CV_MIN_VALUE 0
#define CV_MAX_VALUE 7000

// -4 .. +4 V
//GAIN: 3500 OFFSET: 0       -3.96
//GAIN: 3500 OFFSET: 3500     0.0
//GAIN: 0    OFFSET: 3500     4.01
// 8 Octave => 8 * 12 = 96 notes
// 8 Octave => 7000 DAC values => 7000 / 96 = 72 DAC values between two consecutive note

#define MOD_AMT     1
#define MOD_AMT_GAIN 0
#define MOD_AMT_MIN_VALUE 0
#define MOD_AMT_MAX_VALUE 3500
// 0 .. +4 V
//GAIN: 0    OFFSET: 0        0
//GAIN: 0    OFFSET: 3500     4.01

#define WAVE_SELECT 2
#define WAVE_SELECT_GAIN 1000
#define WAVE_SELECT_MIN_VALUE 0
#define WAVE_SELECT_MAX_VALUE 4000
// -2 .. +4 V
//GAIN: 1000 OFFSET: 0       -1.12 SQR
//GAIN: 1000 OFFSET: 500     -0.54
//GAIN: 1000 OFFSET: 1000     0.0
//GAIN: 1000 OFFSET: 1500     0.58 TRI
//GAIN: 1000 OFFSET: 2000     1.15
//GAIN: 1000 OFFSET: 2500     1.72 TRI + SAW
//GAIN: 1000 OFFSET: 3000     2.29
//GAIN: 1000 OFFSET: 3500     2.87
//GAIN: 1000 OFFSET: 4000     3.44 SAW
// Might be a 4 position switch ?
// -2 V < SQR > -0.3 (+/-0.15) V < TRI > +1.25 (+/-0.3) V < TRI + SAW > +2.7 (+/-0.5) V < SAW > 4V

#define PWM         3
#define PWM_GAIN    0
#define PWM_MIN_VALUE     0
#define PWM_MAX_VALUE     2000
// 0 .. +2.2 V
//GAIN: 0    OFFSET: 0        0
//GAIN: 0    OFFSET: 2000     2.29

#define MIXER       4
#define MIXER_GAIN  1500
#define MIXER_MIN_VALUE       0
#define MIXER_MAX_VALUE       3000
// -2 .. 2 V
//GAIN: 1500 OFFSET: 0       -1.68
//GAIN: 1500 OFFSET: 3000     1.72

#define RESONANCE   5
#define RESONANCE_GAIN 0
#define RESONANCE_MIN_VALUE   0
#define RESONANCE_MAX_VALUE   2000
// 0 .. +2.5 V
//GAIN: 0    OFFSET: 0        0
//GAIN: 0    OFFSET: 2000     2.29

#define CUTOFF      6
#define CUTOFF_GAIN_LOW 2500
#define CUTOFF_GAIN_THRESHOLD 2500
#define CUTOFF_GAIN_HIGH 0
#define VCF_MIN_VALUE 0
#define VCF_MAX_VALUE 6000
// -3 .. +4 V
//GAIN: 2500 OFFSET: 0       -2.83
//GAIN: 2500 OFFSET: 2500     0.0
//GAIN: 0    OFFSET: 0        0.0
//GAIN: 0    OFFSET: 3500     4.01
// 6000 DAC values

#define VCA         7
#define VCA_GAIN    0
#define VCA_MIN_VALUE 0
#define VCA_MAX_VALUE 3500
// 0 .. +4.3 V
//GAIN: 0    OFFSET: 0        0
//GAIN: 0    OFFSET: 3500     4.01

#define LFO_SAW 0
#define LFO_RMP 1
#define LFO_TRI 2
#define LFO_SQR 3
#define LFO_PHASE_UP 0
#define LFO_PHASE_DOWN 1
#define LFO_MIN_VALUE 0
#define LFO_MAX_VALUE 1024

typedef struct lfo_structure {
  uint8_t shape;
  uint8_t phase;
  uint16_t rate;
  uint16_t min_value;
  uint16_t max_value;
  uint16_t accumulator;
  uint16_t value;
} lfo;

#define ENV_IDLE 0
#define ENV_ATTACK 1
#define ENV_DECAY 2
#define ENV_SUSTAIN 3
#define ENV_RELEASE 4

typedef struct envelope_structure {
  uint8_t state;
  uint16_t min_value;
  uint16_t max_value;
  uint16_t value;
  uint16_t attack_rate;
  uint16_t decay_rate;
  uint16_t sustain_value;
  uint16_t release_rate;
} envelope;

void updatePitch(uint16_t pitch);
void updateModAmount(uint16_t modAmount);
void updateWaveSelect(uint16_t waveSelect);
void updatePwm(uint16_t pwm);
void updateMixer(uint16_t mixer);
void updateResonance(uint16_t resonance);
void updateCutoff(uint16_t cutoff);
void updateVca(uint16_t vca);

void (*updateVoiceParam[8])(uint16_t) = {
  updatePitch,
  updateModAmount,
  updateWaveSelect,
  updatePwm,
  updateMixer,
  updateResonance,
  updateCutoff,
  updateVca
};

typedef struct voice_structure {
  envelope_structure vca_envelope = {
    ENV_IDLE, // state
    VCA_MIN_VALUE, // min_value
    VCA_MAX_VALUE, // max_value
    VCA_MIN_VALUE, // value 0 - 3500
    100,  // attack_rate
    200,  // decay_rate
    2500, // sustain_value
    50  // release_rate
  };
  envelope_structure vcf_envelope = {
    ENV_IDLE,
    VCF_MIN_VALUE, // min_value
    VCF_MAX_VALUE, // max_value
    VCF_MIN_VALUE, // value 0 - 6000
    66,  // attack_rate
    400,  // decay_rate
    5000, // sustain_value
    30  // release_rate
  };
  lfo_structure lfo = {
    LFO_TRI,       // shape
    LFO_PHASE_UP,  // phase
    10,           // rate
    LFO_MIN_VALUE, // min_value
    LFO_MAX_VALUE, // max_value
    LFO_MIN_VALUE, // accumulator
    LFO_MIN_VALUE  // value
  };
  uint16_t lfoModAmount[8] {
    0, // LFO to PITCH
    0, // LFO to MOD_AMT
    0, // LFO to WAVE SELECT
    0, // LFO to PWM
    0, // LFO to MIXER
    0, // LFO to RESONANCE
    0, // LFO to VCF
    0  // LFO to VCA
  };
  uint16_t dacValues[8] = {
    CV_MAX_VALUE / 2,          //   0..7000,   // Key CV
    MOD_AMT_MAX_VALUE / 2,     //   0..3500,   // Mod Amount
    WAVE_SELECT_MAX_VALUE / 2, //   0..4000,   // Wave Select
    PWM_MAX_VALUE / 2,         //   0..2000,   // PWM
    MIXER_MAX_VALUE / 2,       //   0..3500,   // Mixer Balance
    RESONANCE_MAX_VALUE / 2,   //   0..2000,   // Resonance
    VCF_MAX_VALUE / 2,         //   0..6000,   // Cutoff
    VCA_MAX_VALUE /2           //   0..3500    // VCA
  };
  uint8_t gate;
} voice;

voice voicess[6];

void printVoiceParams(voice *voice) {
  Serial.print("Voice: ");Serial.print(voice->gate);
  Serial.print("\tVCA_STATE: ");Serial.print(voice->vca_envelope.state);
  Serial.print("\tVCA_VALUE: ");Serial.print(voice->vca_envelope.value);
  Serial.print("\tVCF_STATE: ");Serial.print(voice->vcf_envelope.state);
  Serial.print("\tVCF_VALUE: ");Serial.print(voice->vcf_envelope.value);

  Serial.print("\tLFO_SHAPE: ");Serial.print(voice->lfo.shape);
  Serial.print("\tLFO_ACC: ");Serial.print(voice->lfo.accumulator);
  Serial.print("\tLFO_VALUE: ");Serial.println(voice->lfo.value);
}

void printLfoParams(voice *voice) {
  Serial.print("LFO_SHAPE: ");Serial.print(voice->lfo.shape);
  Serial.print("\tLFO_ACC: ");Serial.print(voice->lfo.accumulator);
  Serial.print("\tLFO_VALUE: ");Serial.println(voice->lfo.value);
}

void updateLfo(voice *voice) {
  voice->lfo.accumulator += voice->lfo.rate;
  if(voice->lfo.accumulator >= voice->lfo.max_value)
    voice->lfo.accumulator = voice->lfo.min_value;

  if(voice->lfo.accumulator < voice->lfo.max_value >> 1)
    voice->lfo.phase = LFO_PHASE_UP;
  else
    voice->lfo.phase = LFO_PHASE_DOWN;

  switch(voice->lfo.shape){
    case LFO_SAW:
      voice->lfo.value = voice->lfo.max_value - voice->lfo.accumulator;
      break;
    case LFO_RMP:
      voice->lfo.value = voice->lfo.accumulator;
      break;
    case LFO_TRI:
      if(voice->lfo.phase == LFO_PHASE_UP)
        voice->lfo.value = voice->lfo.accumulator << 1;
      else
        voice->lfo.value = voice->lfo.max_value - voice->lfo.accumulator << 1;
      break;
    case LFO_SQR:
      if(voice->lfo.phase == LFO_PHASE_UP)
        voice->lfo.value = voice->lfo.min_value;
      else
        voice->lfo.value = voice->lfo.max_value;
      break;
    default:
    break;
  }
}

// issue when *_rate is 0!
void updateEnvelope(envelope_structure *envelope, voice *voice) {
  // Check gate
  switch(envelope->state) {
    case ENV_IDLE:
      if(voice->gate){ // note on
        envelope->state = ENV_ATTACK;
        envelope->value = envelope->min_value;
      }
      break;
    case ENV_ATTACK:
      if(voice->gate) {
        envelope->value += envelope->attack_rate;
        if(envelope->value >= envelope->max_value){
          envelope->value = envelope->max_value;
          envelope->state = ENV_DECAY;
        }
      } else {
        envelope->state = ENV_DECAY;
      }
      break;
    case ENV_DECAY:
      envelope->value -= envelope->decay_rate;
      if(voice->gate) {
        if(envelope->value <= envelope->sustain_value){
          // reached sustain level and gate is still on
          envelope->state = ENV_SUSTAIN;
          envelope->value = envelope->sustain_value;
        }
      } else {
        // go to release or decay away?
        if(envelope->value <= envelope->min_value){
          envelope->value = envelope->min_value;
          envelope->state = ENV_IDLE;
        }
      }
      break;
    case ENV_SUSTAIN:
      if(voice->gate) {
        envelope->value = envelope->sustain_value;
      } else {
        envelope->state = ENV_RELEASE;
      }
      break;
    case ENV_RELEASE:
      envelope->value -= envelope->release_rate;
      if(envelope->value < envelope->release_rate){
        envelope->value = envelope->min_value;
        envelope->state = ENV_IDLE;
      }
      break;
  }
}

void voiceNoteOn(voice *voice) {
  voice->gate = 1;
  voice->vca_envelope.state = ENV_ATTACK;
  voice->vcf_envelope.state = ENV_ATTACK;
}
void voiceNoteOff(voice *voice) {
  voice->gate = 0;
}

// MP4922 pin 1  +5V
// MP4922 pin 2  NC
// MP4922 pin 3  ~CS / Arduino pin 7
// MP4922 pin 4  SCK / Arduino 13 - ICSP/SCK/3
// MP4922 pin 5  SDI / Arduino 11 - ICSP/MOSI/4
// MP4922 pin 6  NC
// MP4922 pin 7  NC

// MP4922 pin 8  ~LDAC / GND
// MP4922 pin 9  ~SHDN / NC?
// MP4922 pin 10 Voutb / 10k / Opamp Non-inverting input
// MP4922 pin 11 Vrefb / +5V
// MP4922 pin 12 GND
// MP4922 pin 13 Vrefa / +5V
// MP4922 pin 14 Vouta / 10k / Opamp Inverting input / 4051 pin 3

// Voice Select 4051

// 4051 pin 1  | out 4  | Voice 4 4051 pin 6
// 4051 pin 2  | out 6  | NC (Voice 6 4051 pin 6)
// 4051 pin 3  | COMMON | MP4922 pin 14
// 4051 pin 4  | out 7  | NC (Voice 7 4051 pin 6)
// 4051 pin 5  | out 5  | Voice 5 4051 pin 6
// 4051 pin 6  | INH    | Arduino pin 6 (Voice Select)
// 4051 pin 7  | Vee    | NC
// 4051 pin 8  | Vss    | GND

// 4051 pin 9  | C      | Arduino pin 10
// 4051 pin 10 | B      | Arduino pin 9
// 4051 pin 11 | A      | Arduino pin 8
// 4051 pin 12 | out 3  | Voice 3 4051 pin 6
// 4051 pin 13 | out 0  | Voice 0 4051 pin 6
// 4051 pin 14 | out 1  | Voice 1 4051 pin 6
// 4051 pin 15 | out 2  | Voice 2 4051 pin 6
// 4051 pin 16 | Vdd    | +5V

// Voice Param 4051 (x6)

// 4051 pin 1  | out 4  | AS3394 pin 13 MIXER
// 4051 pin 2  | out 6  | AS3394 pin 20 CUTOFF
// 4051 pin 3  | COMMON | MP4922 pin 14
// 4051 pin 4  | out 7  | AS3394 pin 22 VCA
// 4051 pin 5  | out 5  | AS3394 pin 15 RESONANCE
// 4051 pin 6  | INH    | Arduino pin 6 (Voice Select 4051 pin 13/14/15/12/1/5/2/4)
// 4051 pin 7  | Vee    | NC
// 4051 pin 8  | Vss    | GND

// 4051 pin 9  | C      | Arduino pin 10
// 4051 pin 10 | B      | Arduino pin 9
// 4051 pin 11 | A      | Arduino pin 8
// 4051 pin 12 | out 3  | AS3394 pin 8 PWM
// 4051 pin 13 | out 0  | AS3394 pin 2 CV
// 4051 pin 14 | out 1  | AS3394 pin 6 MOD_AMT
// 4051 pin 15 | out 2  | AS3394 pin 7 WAVE_SELECT
// 4051 pin 16 | Vdd    | +5V

static constexpr unsigned k_pinDisableMultiplex = 6;
static constexpr unsigned k_pinChipSelectDAC = 7;

OutputPort<PORT_B> voiceParamSelect;                    // Arduino pin 8, 9, 10 to 4051 11, 10, 9
OutputPort<PORT_D, 2, 3> voiceSelectPort;               // Arduino pin 3, 4, 5
Output<k_pinChipSelectDAC>  m_outputChipSelectDAC;      // Arduino pin 7 to MP4922 pin 3
Output<k_pinDisableMultiplex> m_outputDisableMultiplex; // Arduino pin 6 to 4051 pin 6

#define DAC_A_GAIN 1 // 0: x2, 1: x1
#define DAC_B_GAIN 1 // 0: x2, 1: x1

#define DAC_A_BUFFER 1 // 0: Unbuffered, 1: Buffered
#define DAC_B_BUFFER 1 // 0: Unbuffered, 1: Buffered

#define DAC_A_SHDN 1 // 0: Channel off, 1: Channel on
#define DAC_B_SHDN 1 // 0: Channel off, 1: Channel on

#define k_writeChannelA (((0x0 << 7) | (DAC_A_BUFFER << 6) | (DAC_A_GAIN << 5) | (DAC_A_SHDN << 4)) << 8)
#define k_writeChannelB (((0x1 << 7) | (DAC_B_BUFFER << 6) | (DAC_B_GAIN << 5) | (DAC_B_SHDN << 4)) << 8)

uint8_t timer1Prescaler[] = {
  (0 << CS12) | (1 << CS11) | (1 << CS10), // 64
  (1 << CS12) | (0 << CS11) | (0 << CS10), // 256
  (1 << CS12) | (0 << CS11) | (1 << CS10)  // 1024
};

uint8_t timer2Prescaler[] = {
  (0 << CS22) | (1 << CS21) | (1 << CS20), // 32
  (1 << CS22) | (0 << CS21) | (0 << CS20), // 64
  (1 << CS22) | (0 << CS21) | (1 << CS20), // 128
  (1 << CS22) | (1 << CS21) | (0 << CS20), // 256
  (1 << CS22) | (1 << CS21) | (1 << CS20)  // 1024
};

uint16_t bufferB[10];
uint8_t data_available_B = 0;

#define TIMER1_PRESCALER timer1Prescaler[1]
#define TIMER1_COUNTER ((12000000 / (256 * 1000)) - 1)

#define TIMER2_PRESCALER timer2Prescaler[4]
#define TIMER2_COUNTER ((12000000 / (1024 * 10)) - 1)

void initializeInterrupts() {
  cli();                                    // stop interrupts

  // TIMER 1 for interrupt frequency 1000 Hz:
  TIMSK1 = 0;                               // disable timer compare interrupt
  TCCR1A = 0;                               // set entire TCCR1A register to 0
  TCCR1B = 0;                               // same for TCCR1B
  TCNT1  = 0;                               // initialize counter value to 0
                                            // set compare match register for 500 Hz increments
                                            // = 12000000 / (256 * 500) - 1 (must be < 65536)
  OCR1A = 186;
  TCCR1B |= (1 << WGM12);                   // turn on CTC mode
  TCCR1B |= TIMER1_PRESCALER;               // Set CS12, CS11 and CS10 bits for prescaler
  TIMSK1 |= (1 << OCIE1A);                  // enable timer compare interrupt

  // TIMER 2 for interrupt frequency 120 Hz:
  TIMSK2 = 0;                               // disable timer compare interrupt
  TCCR2A = 0;                               // set entire TCCR2A register to 0
  TCCR2B = 0;                               // set entire TCCR2B register to 0
  TCNT2  = 0;                               // initialize counter value to 0
                                            // set compare match register for 120 Hz increments (must be < 65536)
                                            // = 12000000 / (256 * 120) - 1 <= 256
  OCR2A = TIMER2_COUNTER;
  TCCR2A |= (1 << WGM21);                   // turn on CTC mode
  TCCR2B |= TIMER2_PRESCALER;               // Set CS22, CS21 and CS20 bits for 256 prescaler
  TIMSK2 |= (1 << OCIE2A);                  // enable timer compare interrupt

  sei();                                    // allow interrupts
}

uint8_t irq1Count = 0;

uint8_t voiceNumber = 0;
uint8_t voiceParamNumber = 0;

ISR(TIMER1_COMPA_vect) {                // interrupt commands for TIMER 1
  voiceNumber = irq1Count >> 3;         // 0..5 => 0b000xxx ... 0b101xxx
  voiceParamNumber = irq1Count & 0x07;  // 0..7 => 0bxxx000 ... 0bxxx111

  m_outputDisableMultiplex = HIGH;          // Disable Multiplex

  updateVoiceParam[voiceParamNumber](voicess[voiceNumber].dacValues[voiceParamNumber]);
  voiceSelectPort.write(voiceNumber);
  voiceParamSelect.write(voiceParamNumber); // Voice and Param Select

  m_outputDisableMultiplex = LOW;           // Enable Multiplex

  // Serial.println(irq1Count);
  irq1Count++;
  if(irq1Count > 47)
    irq1Count = 0;
}

uint8_t irq2Count = 0;

ISR(TIMER2_COMPA_vect){                 // interrupt commands for TIMER 2 here
  // Update Envelope
  updateEnvelope(&voicess[0].vca_envelope, &voicess[0]);
  updateEnvelope(&voicess[0].vcf_envelope, &voicess[0]);
  updateLfo(&voicess[0]);

  if(irq2Count == 255){
    voicess[0].gate == 1 ? voiceNoteOff(&voicess[0]) : voiceNoteOn(&voicess[0]);
  }

  irq2Count++;
}

void setup() {

  m_outputChipSelectDAC = LOW;     // Disable DAC (~CS)
  m_outputDisableMultiplex = HIGH; // Disable Multiplex

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  Serial.begin(115200);
  Serial.println("Hello world");
  initializeInterrupts();
}

void loop() {
  delay(10);
  // printVoiceParams(&voicess[0]);

//  Serial.print(voicess[0].vca_envelope.value); Serial.print(" ");
//  Serial.print(voicess[0].vcf_envelope.value); Serial.print(" ");
//  Serial.print(voicess[1].vca_envelope.value); Serial.print(" ");
//  Serial.print(voicess[1].vcf_envelope.value); Serial.println();
}

void updatePitch(uint16_t pitch) {

// -4 .. +4 V

// GAIN: 3500 OFFSET: 0       -3.96
// GAIN: 3500 OFFSET: 3500     0.0
// GAIN: 0    OFFSET: 3500     4.01
// 8 Octave => 8 * 12 = 96 notes
// 8 Octave => 7000 DAC values => 7000 / 96 = 72 DAC values between two consecutive note

  uint16_t gain = pitch > 3500 ? 0 : 3500;
  uint16_t offset = pitch > 3500 ? (pitch - 3500) : pitch;

  // Serial.print("CV: ");Serial.print(pitch);

  updateDAC(gain, offset);
}

void updateModAmount(uint16_t modAmount) {
  // Serial.print("|MOD: ");Serial.print(modAmount);
  updateDAC(MOD_AMT_GAIN, constrain(modAmount,
                                    MOD_AMT_MIN_VALUE,
                                    MOD_AMT_MAX_VALUE));
}

void updateWaveSelect(uint16_t waveSelect) {
  // Serial.print("|WS: ");Serial.print(waveSelect);
  updateDAC(WAVE_SELECT_GAIN, constrain(waveSelect,
                                        WAVE_SELECT_MIN_VALUE,
                                        WAVE_SELECT_MAX_VALUE));
}

void updatePwm(uint16_t pwm){
  // Serial.print("|PWM: ");Serial.print(pwm);
  updateDAC(PWM_GAIN, constrain(pwm,
                                PWM_MIN_VALUE,
                                PWM_MAX_VALUE));
}

void updateMixer(uint16_t mixer) {
  // Serial.print("|MIX: ");Serial.print(mixer);
  updateDAC(MIXER_GAIN, constrain(mixer,
                                  MIXER_MIN_VALUE,
                                  MIXER_MAX_VALUE));
}

void updateResonance(uint16_t resonance) {
  // Serial.print("|RES: ");Serial.print(resonance);
  updateDAC(RESONANCE_GAIN, constrain(resonance,
                                      RESONANCE_MIN_VALUE,
                                      RESONANCE_MAX_VALUE));
}

void updateCutoff(uint16_t cutoff) {
  // Serial.print("|VCF: ");Serial.print(cutoff);
  if(cutoff > CUTOFF_GAIN_THRESHOLD) {
    updateDAC(CUTOFF_GAIN_HIGH, constrain((cutoff - CUTOFF_GAIN_THRESHOLD),
                                          VCF_MIN_VALUE,
                                          VCF_MAX_VALUE - CUTOFF_GAIN_THRESHOLD));
  } else {
    updateDAC(CUTOFF_GAIN_LOW, constrain(cutoff,
                                         VCF_MIN_VALUE,
                                         CUTOFF_GAIN_THRESHOLD));
  }
}

void updateVca(uint16_t vca) {
  // Serial.print("|VCA: ");Serial.print(vca);
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
  m_outputChipSelectDAC = LOW;     // Disable DAC latch

  command = k_writeChannelB | ((offset >> 1) & 0x0FFF);

  SPI.transfer(command >> 8);
  SPI.transfer(command & 0xFF);

  m_outputChipSelectDAC = HIGH;    // DAC latch Vout
}
