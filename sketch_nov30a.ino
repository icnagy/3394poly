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
    VCA_MIN_VALUE, // value
    100,  // attack_rate
    200,  // decay_rate
    2500, // sustain_value
    50,  // release_rate
  }; // 0 - 3500
  envelope_structure vcf_envelope = {
    ENV_IDLE,
    VCF_MIN_VALUE, // min_value
    VCF_MAX_VALUE, // max_value
    VCF_MIN_VALUE, // value
    66,  // attack_rate
    400,  // decay_rate
    5000, // sustain_value
    30,  // release_rate
  }; // 0 - 6000
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

voice voicess[8];

void printVoiceParams(voice *voice) {
  Serial.print("Gate: ");Serial.print(voice->gate);
  Serial.print("\tVCA_ENV_STATE: ");Serial.print(voice->vca_envelope.state);
  Serial.print("\tVCA_ENV_VALUE: ");Serial.print(voice->vca_envelope.value);
  Serial.print("\tVCF_ENV_STATE: ");Serial.print(voice->vcf_envelope.state);
  Serial.print("\tVCF_ENV_VALUE: ");Serial.println(voice->vcf_envelope.value);

//  Serial.print("VCA_ENV_MIN_VALUE: ");Serial.println(voice->vca_envelope.min_value);
//  Serial.print("VCA_ENV_MAX_VALUE: ");Serial.println(voice->vca_envelope.max_value);
//  Serial.print("VCA_ENV_ATTACK_RATE: ");Serial.println(voice->vca_envelope.attack_rate);
//  Serial.print("VCA_ENV_DECAY_RATE: ");Serial.println(voice->vca_envelope.decay_rate);
//  Serial.print("VCA_ENV_SUSTAIN_VALUE: ");Serial.println(voice->vca_envelope.sustain_value);
//  Serial.print("VCA_ENV_RELEASE_RATE: ");Serial.println(voice->vca_envelope.release_rate);
}

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

#define LFO_SAW 0
#define LFO_SINE 1
#define LFO_TRI 2

static uint16_t waveForm[3][120] =   {
  // Sawtooth wave
  {
    0x022, 0x044, 0x066, 0x088, 0x0aa, 0x0cc, 0x0ee, 0x110, 0x132, 0x154,
    0x176, 0x198, 0x1ba, 0x1dc, 0x1fe, 0x220, 0x242, 0x264, 0x286, 0x2a8,
    0x2ca, 0x2ec, 0x30e, 0x330, 0x352, 0x374, 0x396, 0x3b8, 0x3da, 0x3fc,
    0x41e, 0x440, 0x462, 0x484, 0x4a6, 0x4c8, 0x4ea, 0x50c, 0x52e, 0x550,
    0x572, 0x594, 0x5b6, 0x5d8, 0x5fa, 0x61c, 0x63e, 0x660, 0x682, 0x6a4,
    0x6c6, 0x6e8, 0x70a, 0x72c, 0x74e, 0x770, 0x792, 0x7b4, 0x7d6, 0x7f8,
    0x81a, 0x83c, 0x85e, 0x880, 0x8a2, 0x8c4, 0x8e6, 0x908, 0x92a, 0x94c,
    0x96e, 0x990, 0x9b2, 0x9d4, 0x9f6, 0xa18, 0xa3a, 0xa5c, 0xa7e, 0xaa0,
    0xac2, 0xae4, 0xb06, 0xb28, 0xb4a, 0xb6c, 0xb8e, 0xbb0, 0xbd2, 0xbf4,
    0xc16, 0xc38, 0xc5a, 0xc7c, 0xc9e, 0xcc0, 0xce2, 0xd04, 0xd26, 0xd48,
    0xd6a, 0xd8c, 0xdae, 0xdd0, 0xdf2, 0xe14, 0xe36, 0xe58, 0xe7a, 0xe9c,
    0xebe, 0xee0, 0xf02, 0xf24, 0xf46, 0xf68, 0xf8a, 0xfac, 0xfce, 0xffe
  },
  // Sine wave
  {
    0x000, 0x044, 0x088, 0x0cc, 0x110, 0x154, 0x198, 0x1dc, 0x220, 0x264,
    0x2a8, 0x2ec, 0x330, 0x374, 0x3b8, 0x3fc, 0x440, 0x484, 0x4c8, 0x50c,
    0x550, 0x594, 0x5d8, 0x61c, 0x660, 0x6a4, 0x6e8, 0x72c, 0x770, 0x7b4,
    0x7f8, 0x83c, 0x880, 0x8c4, 0x908, 0x94c, 0x990, 0x9d4, 0xa18, 0xa5c,
    0xaa0, 0xae4, 0xb28, 0xb6c, 0xbb0, 0xbf4, 0xc38, 0xc7c, 0xcc0, 0xd04,
    0xd48, 0xd8c, 0xdd0, 0xe14, 0xe58, 0xe9c, 0xee0, 0xf24, 0xf68, 0xfac,
    0xffe, 0xfac, 0xf68, 0xf24, 0xee0, 0xe9c, 0xe58, 0xe14, 0xdd0, 0xd8c,
    0xd48, 0xd04, 0xcc0, 0xc7c, 0xc38, 0xbf4, 0xbb0, 0xb6c, 0xb28, 0xae4,
    0xaa0, 0xa5c, 0xa18, 0x9d4, 0x990, 0x94c, 0x908, 0x8c4, 0x880, 0x83c,
    0x7f8, 0x7b4, 0x770, 0x72c, 0x6e8, 0x6a4, 0x660, 0x61c, 0x5d8, 0x594,
    0x550, 0x50c, 0x4c8, 0x484, 0x440, 0x3fc, 0x3b8, 0x374, 0x330, 0x2ec,
    0x2a8, 0x264, 0x220, 0x1dc, 0x198, 0x154, 0x110, 0x0cc, 0x088, 0x044
  },
  // Triangle wave
  {
    0x7ff, 0x86a, 0x8d5, 0x93f, 0x9a9, 0xa11, 0xa78, 0xadd, 0xb40, 0xba1,
    0xbff, 0xc5a, 0xcb2, 0xd08, 0xd59, 0xda7, 0xdf1, 0xe36, 0xe77, 0xeb4,
    0xeec, 0xf1f, 0xf4d, 0xf77, 0xf9a, 0xfb9, 0xfd2, 0xfe5, 0xff3, 0xffc,
    0xfff, 0xffc, 0xff3, 0xfe5, 0xfd2, 0xfb9, 0xf9a, 0xf77, 0xf4d, 0xf1f,
    0xeec, 0xeb4, 0xe77, 0xe36, 0xdf1, 0xda7, 0xd59, 0xd08, 0xcb2, 0xc5a,
    0xbff, 0xba1, 0xb40, 0xadd, 0xa78, 0xa11, 0x9a9, 0x93f, 0x8d5, 0x86a,
    0x7ff, 0x794, 0x729, 0x6bf, 0x655, 0x5ed, 0x586, 0x521, 0x4be, 0x45d,
    0x3ff, 0x3a4, 0x34c, 0x2f6, 0x2a5, 0x257, 0x20d, 0x1c8, 0x187, 0x14a,
    0x112, 0x0df, 0x0b1, 0x087, 0x064, 0x045, 0x02c, 0x019, 0x00b, 0x002,
    0x000, 0x002, 0x00b, 0x019, 0x02c, 0x045, 0x064, 0x087, 0x0b1, 0x0df,
    0x112, 0x14a, 0x187, 0x1c8, 0x20d, 0x257, 0x2a5, 0x2f6, 0x34c, 0x3a4,
    0x3ff, 0x45d, 0x4be, 0x521, 0x586, 0x5ed, 0x655, 0x6bf, 0x729, 0x794
  }
};
int lfoWavePointer = 0;

static constexpr unsigned k_pinDisableMultiplex = 6;
static constexpr unsigned k_pinChipSelectDAC = 7;

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

OutputPort<PORT_B> voiceParamSelect;                    // Arduino pin 8, 9, 10 to 4051 11, 10, 9
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

uint8_t voiceSelect = 0;
uint8_t voiceParamNumber = 0;

ISR(TIMER1_COMPA_vect) {                // interrupt commands for TIMER 1
  voiceSelect = irq1Count >> 3;         // 0..5 => 0b000xxx ... 0b101xxx
  voiceParamNumber = irq1Count & 0x07;  // 0..7 => 0bxxx000 ... 0bxxx111

  m_outputDisableMultiplex = HIGH;          // Disable Multiplex

  updateVoiceParam[voiceParamNumber](voicess[voiceSelect].dacValues[voiceParamNumber]);
  voiceParamSelect.write(irq1Count);        // Voice and Param Select

  m_outputDisableMultiplex = LOW;           // Enable Multiplex

  irq1Count++;
  if(irq1Count > 48)
    irq1Count = 0;
}

uint8_t irq2Count = 0;

ISR(TIMER2_COMPA_vect){                 // interrupt commands for TIMER 2 here
  // Update Envelope
  updateEnvelope(&voicess[0].vca_envelope, &voicess[0]);
  updateEnvelope(&voicess[0].vcf_envelope, &voicess[0]);
//  updateEnvelope(&voicess[1].vca_envelope, &voicess[1]);
//  updateEnvelope(&voicess[1].vcf_envelope, &voicess[1]);

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
  printVoiceParams(&voicess[0]);

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
  updateDAC(gain, offset);
}

void updateModAmount(uint16_t modAmount) {
  updateDAC(MOD_AMT_GAIN, constrain(modAmount,
                                    MOD_AMT_MIN_VALUE,
                                    MOD_AMT_MAX_VALUE));
}

void updateWaveSelect(uint16_t waveSelect) {
  updateDAC(WAVE_SELECT_GAIN, constrain(waveSelect,
                                        WAVE_SELECT_MIN_VALUE,
                                        WAVE_SELECT_MAX_VALUE));
}

void updatePwm(uint16_t pwm){
  updateDAC(PWM_GAIN, constrain(pwm,
                                PWM_MIN_VALUE,
                                PWM_MAX_VALUE));
}

void updateMixer(uint16_t mixer) {
  updateDAC(MIXER_GAIN, constrain(mixer,
                                  MIXER_MIN_VALUE,
                                  MIXER_MAX_VALUE));
}

void updateResonance(uint16_t resonance) {
  updateDAC(RESONANCE_GAIN, constrain(resonance,
                                      RESONANCE_MIN_VALUE,
                                      RESONANCE_MAX_VALUE));
}

void updateCutoff(uint16_t cutoff) {

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
  updateDAC(VCA_GAIN, constrain(vca,
                                VCA_MIN_VALUE,
                                VCA_MAX_VALUE));
}

void updateDAC(uint16_t gain, uint16_t offset) {
  uint16_t command = k_writeChannelA | (gain & 0x0FFF);

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
