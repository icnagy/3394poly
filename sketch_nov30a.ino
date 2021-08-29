#include <SPI.h>
#include <DirectIO.h>
#include "miby.h"

#define VREF 5
#define DAC_STEPS 4096
#define VOLT_PER_OCTAVE_NOTE 0.75/12
#define DAC_STEP_PER_NOTE (VOLT_PER_OCTAVE_NOTE) / (VREF / DAC_STEPS)

#define NUMBER_OF_VOICES 6
#define NUMBER_OF_PARAMS 8

enum DAC_PARAM {
  DAC_PARAM_CV,
  DAC_PARAM_MOD_AMT,
  DAC_PARAM_WAVE_SELECT,
  DAC_PARAM_PWM,
  DAC_PARAM_MIXER,
  DAC_PARAM_CUTOFF,
  DAC_PARAM_RESONANCE,
  DAC_PARAM_VCA,
  DAC_PARAM_LAST
};

#define CV                    0
#define CV_GAIN_LOW           3500
#define CV_GAIN_SWITCH_POINT  3500
#define CV_GAIN_HIGH          0
#define CV_MIN_VALUE          0
#define CV_MAX_VALUE          7000

// -4 .. +4 V
// GAIN: 3500 OFFSET: 0       -3.96
// GAIN: 3500 OFFSET: 3500     0.0
// GAIN: 0    OFFSET: 3500     4.01
// 8 Octave => 8 * 12 = 96 notes
// 8 Octave => 7000 DAC values => 7000 / 96 = 72 DAC values between two consecutive note

#define MOD_AMT           1
#define MOD_AMT_GAIN      0
#define MOD_AMT_MIN_VALUE 0
#define MOD_AMT_MAX_VALUE 3500
// 0 .. +4 V
//GAIN: 0    OFFSET: 0        0
//GAIN: 0    OFFSET: 3500     4.01

#define WAVE_SELECT             2
#define WAVE_SELECT_GAIN        1000
#define WAVE_SELECT_MIN_VALUE   0
#define WAVE_SELECT_MAX_VALUE   4000
#define WAVE_SELECT_ZERO_VALUE  1000
#define WAVE_SELECT_SQR         0
#define WAVE_SELECT_TRI         1500
#define WAVE_SELECT_TRI_SAW     2500
#define WAVE_SELECT_SAW         4000
// -2 .. +4 V
// GAIN: 1000 OFFSET: 0       -1.12 SQR
// GAIN: 1000 OFFSET: 500     -0.54
// GAIN: 1000 OFFSET: 1000     0.0
// GAIN: 1000 OFFSET: 1500     0.58 TRI
// GAIN: 1000 OFFSET: 2000     1.15
// GAIN: 1000 OFFSET: 2500     1.72 TRI + SAW
// GAIN: 1000 OFFSET: 3000     2.29
// GAIN: 1000 OFFSET: 3500     2.87
// GAIN: 1000 OFFSET: 4000     3.44 SAW
// Might be a 4 position switch ?
// -2 V < SQR > -0.3 (+/-0.15) V < TRI > +1.25 (+/-0.3) V < TRI + SAW > +2.7 (+/-0.5) V < SAW > 4V

#define PWM           3
#define PWM_GAIN      100
#define PWM_MIN_VALUE 0
#define PWM_MAX_VALUE 2000
// 0 .. +2.2 V
// GAIN: 0    OFFSET: 0        0
// GAIN: 0    OFFSET: 2000     2.29

#define MIXER            4
#define MIXER_GAIN       1500
#define MIXER_MIN_VALUE  0
#define MIXER_MAX_VALUE  3000
#define MIXER_ZERO_VALUE 1500
// -2 .. 2 V
// GAIN: 1500 OFFSET: 0       -1.68 VCA2 FULL VCA1 OFF
// GAIN: 1500 OFFSET: 1500     0.0  VCA2 -6dB VCA1 -6dB
// GAIN: 1500 OFFSET: 3000     1.72 VCA2 OFF  VCA1 FULL

#define RESONANCE           5
#define RESONANCE_GAIN      0
#define RESONANCE_MIN_VALUE 0
#define RESONANCE_MAX_VALUE 2000
// 0 .. +2.5 V
// GAIN: 0    OFFSET: 0        0
// GAIN: 0    OFFSET: 2000     2.29

#define CUTOFF                6
#define CUTOFF_GAIN_LOW       2500
#define CUTOFF_GAIN_THRESHOLD 2500
#define CUTOFF_GAIN_HIGH      0
#define CUTOFF_MIN_VALUE      0
#define CUTOFF_MAX_VALUE      6000
#define CUTOFF_ZERO_VALUE     2500
// -3 .. +4 V
// 3/8V per octave
// GAIN: 2500 OFFSET: 0       -2.83  24kHz
// GAIN: 2500 OFFSET: 2500     0.0 ~1300Hz cutoff point
// GAIN: 0    OFFSET: 0        0.0
// GAIN: 0    OFFSET: 3500     4.01    0Hz
// 6000 DAC values

#define VCA           7
#define VCA_GAIN      0
#define VCA_MIN_VALUE 0
#define VCA_MAX_VALUE 3000
// 0 .. +4.3 V
//GAIN: 0    OFFSET: 0        0
//GAIN: 0    OFFSET: 3500     4.01

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
    10,  // attack_rate
    400,  // decay_rate
    2500, // sustain_value
    5  // release_rate
  };
  envelope_structure vcf_envelope = {
    ENV_IDLE,
    CUTOFF_MIN_VALUE, // min_value
    CUTOFF_MAX_VALUE, // max_value
    CUTOFF_MIN_VALUE, // value 0 - 6000
    6000,  // attack_rate
    1,  // decay_rate
    6000, // sustain_value
    6000  // release_rate
  };
  lfo_structure lfo = {
    LFO_TRI,       // shape
    LFO_PHASE_UP,  // phase
    1,             // rate
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
    0,         //   0..3500,   // Mod Amount
    1000,// WAVE_SELECT_TRI,           //   0..4000,   // Wave Select
    PWM_MIN_VALUE, // PWM_MAX_VALUE / 2,         //   0..2000,   // PWM
    0, // MIXER_MAX_VALUE / 2,       //   0..3500,   // Mixer Balance
    0, // RESONANCE_MAX_VALUE / 2,   //   0..2000,   // Resonance
    0, // CUTOFF_MAX_VALUE / 2,         //   0..6000,   // Cutoff
    VCA_MAX_VALUE - 150, // VCA_MAX_VALUE /2           //   0..3500    // VCA
  };
  uint8_t gate;
  uint8_t note;
  uint8_t velocity;
} voice;

// ZERO Volt DAC output
// uint16_t dacValues[8] = {
//   CV_MAX_VALUE / 2,              // Key CV
//   MOD_AMT_MIN_VALUE,             // Mod Amount
//   WAVE_SELECT_ZERO_VALUE,        // Wave Select
//   PWM_MIN_VALUE,                 // PWM
//   MIXER_ZERO_VALUE,              // Mixer Balance
//   RESONANCE_MIN_VALUE,           // Resonance
//   CUTOFF_ZERO_VALUE,             // Cutoff
//   VCA_MIN_VALUE                  // VCA
// };

voice voicess[6];

#define MIBY_SYSEX_BUF_LEN  ( 32 )

extern void test_rt_timing_clock( miby_this_t );
#define MIBY_HND_RT_CLOCK test_rt_timing_clock

extern void test_rt_start( miby_this_t );
#define MIBY_HND_RT_START test_rt_start

extern void test_rt_continue( miby_this_t );
#define MIBY_HND_RT_CONTINUE  test_rt_continue

extern void test_rt_stop( miby_this_t );
#define MIBY_HND_RT_STOP  test_rt_stop

extern void test_rt_active_sense( miby_this_t );
#define MIBY_HND_RT_ACT_SENSE test_rt_active_sense

extern void miby_rt_system_reset( miby_this_t );
#define MIBY_HND_RT_SYS_RESET miby_rt_system_reset

extern void test_tunereq( miby_this_t );
#define MIBY_HND_SYS_TIMEREQ  test_tunereq

extern void test_mtc( miby_this_t );
#define MIBY_HND_SYS_MTC  test_mtc

extern void test_songpos( miby_this_t );
#define MIBY_HND_SYS_SONGPOS  test_songpos

extern void test_songsel( miby_this_t );
#define MIBY_HND_SYS_SONGSEL  test_songsel

extern void test_sysex( miby_this_t );
#define MIBY_HND_SYS_EX   test_sysex

extern void miby_note_on( miby_this_t );
#define MIBY_HND_NOTE_ON  miby_note_on

extern void miby_note_off( miby_this_t );
#define MIBY_HND_NOTE_OFF miby_note_off

extern void test_poly_at( miby_this_t );
#define MIBY_HND_POLY_AT  test_poly_at

extern void test_cc( miby_this_t );
#define MIBY_HND_CTRL_CHG test_cc

extern void test_pb( miby_this_t );
#define MIBY_HND_PITCHBEND  test_pb

extern void test_pc( miby_this_t );
#define MIBY_HND_PROG_CHG test_pc

extern void test_chanat( miby_this_t );
#define MIBY_HND_CHAN_AT  test_chanat

void printVoiceDacValues(voice *voice) {
  Serial.print("Gate: ");Serial.print(voice->gate);
  Serial.print(" CV: ");Serial.print(voice->dacValues[CV]);
  Serial.print("\tMOD: ");Serial.print(voice->dacValues[MOD_AMT]);
  Serial.print("\tWAV: ");Serial.print(voice->dacValues[WAVE_SELECT]);
  Serial.print("\tPWM: ");Serial.print(voice->dacValues[PWM]);
  Serial.print("\tMIX: ");Serial.print(voice->dacValues[MIXER]);
  Serial.print("\tVCF: ");Serial.print(voice->dacValues[CUTOFF]);
  Serial.print("\tRES: ");Serial.print(voice->dacValues[RESONANCE]);
  Serial.print("\tVCA: ");Serial.println(voice->dacValues[VCA]);
}

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

int findVoiceWithNote(uint8_t note) {
  for(int i = 0; i < NUMBER_OF_VOICES; i++) {
    if(voicess[i].note == note)
      return i;
  }
  return -1;
}

int findIdleVoice() {
  for(int i = 0; i < NUMBER_OF_VOICES; i++) {
    if(voicess[i].vca_envelope.state == ENV_IDLE)
      return i;
  }
  return -1;
}

void voiceNoteOn(int voiceNo, uint8_t note, uint8_t velocity) {
  voicess[voiceNo].gate = 1;
  voicess[voiceNo].note = note;
  voicess[voiceNo].dacValues[CV] = constrain(72 * note, CV_MIN_VALUE, CV_MAX_VALUE);
  voicess[voiceNo].velocity = velocity;
  voicess[voiceNo].vca_envelope.state = ENV_ATTACK;
  voicess[voiceNo].vcf_envelope.state = ENV_ATTACK;
}
void voiceNoteOff(int voiceNo, uint8_t note, uint8_t velocity) {
  voicess[voiceNo].gate = 0;
  voicess[voiceNo].note = note;
  voicess[voiceNo].velocity = velocity;
  voicess[voiceNo].vca_envelope.state = ENV_RELEASE;
  voicess[voiceNo].vcf_envelope.state = ENV_RELEASE;
}

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

// http://www.8bit-era.cz/arduino-timer-interrupts-calculator.html
void initializeInterrupts() {
  cli();                                    // stop interrupts

  // TIMER 1 for interrupt frequency 1142.0413990007137 Hz:
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; // initialize counter value to 0
  // set compare match register for 1142.0413990007137 Hz increments
  OCR1A = 14009; // = 16000000 / (1 * 1142.0413990007137) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12, CS11 and CS10 bits for 1 prescaler
  TCCR1B |= (0 << CS12) | (0 << CS11) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  // TIMER 2 for interrupt frequency 1146.788990825688 Hz:
  TCCR2A = 0; // set entire TCCR2A register to 0
  TCCR2B = 0; // same for TCCR2B
  TCNT2  = 0; // initialize counter value to 0
  // set compare match register for 1146.788990825688 Hz increments
  OCR2A = 217; // = 16000000 / (64 * 1146.788990825688) - 1 (must be <256)
  // turn on CTC mode
  TCCR2B |= (1 << WGM21);
  // Set CS22, CS21 and CS20 bits for 64 prescaler
  TCCR2B |= (1 << CS22) | (0 << CS21) | (0 << CS20);
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);

  sei();                                    // allow interrupts
}

uint8_t irq1Count = 0;
uint16_t irq2Count = 0;

ISR(TIMER1_COMPA_vect) {                // interrupt commands for TIMER 1

  uint8_t voiceNumber = 0;
  uint8_t voiceParamNumber = 0;

  voiceNumber = irq1Count >> 3;         // 0..5 => 0b000xxx ... 0b101xxx
  voiceParamNumber = irq1Count & 0x07;  // 0..7 => 0bxxx000 ... 0bxxx111

  m_outputDisableMultiplex = HIGH;      // Disable Multiplex

  // Send value to DAC
  updateVoiceParam[voiceParamNumber](voicess[voiceNumber].dacValues[voiceParamNumber]);

  voiceSelectPort.write(voiceNumber);   // Voice Select
  voiceParamSelect.write(voiceParamNumber); // VoiceParam Select

  m_outputDisableMultiplex = LOW;       // Enable Multiplex

  irq1Count++;
  if(irq1Count > 23)
    irq1Count = 0;
}

ISR(TIMER2_COMPA_vect){                 // interrupt commands for TIMER 2 here
  // Update Envelope
  for(int i = 0; i < NUMBER_OF_VOICES; i ++) {

    // updateEnvelope(&voicess[i].vca_envelope, &voicess[i]);
    // voicess[i].dacValues[VCA] = voicess[i].vca_envelope.value;

    // updateEnvelope(&voicess[i].vcf_envelope, &voicess[i]);
    // voicess[i].dacValues[CUTOFF] = (CUTOFF_MAX_VALUE - voicess[i].vcf_envelope.value);

    updateLfo(&voicess[i]);
    voicess[i].dacValues[PWM] = voicess[i].lfo.value;
  }

  irq2Count++;
  if(irq2Count > 1146/2){
    irq2Count = 0;
    printVoiceDacValues(&voicess[0]);
    printVoiceDacValues(&voicess[1]);
    printVoiceDacValues(&voicess[2]);
    printVoiceDacValues(&voicess[3]);
    printVoiceDacValues(&voicess[4]);
    printVoiceDacValues(&voicess[5]);
    Serial.println("------------------------------");
  }

}

miby_t m;

void setup() {

  m_outputChipSelectDAC = LOW;     // Disable DAC (~CS)
  m_outputDisableMultiplex = HIGH; // Disable Multiplex

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  Serial.begin(115200);
  Serial.println("Hello world");

  miby_init( &m, NULL );

  panic();

  initializeInterrupts();
}

char rx_byte = 0;
uint8_t selectedVoice = 0;

void loop() {
  if (Serial.available() > 0) {    // is a character available?
    rx_byte = Serial.read();       // get the character

    processSerialInput(rx_byte);

    // miby_parse( &m, rx_byte);
    // if ( MIBY_ERROR_MISSING_DATA(&m) )
    // {
    //   Serial.println( "*** MISSING DATA ***\n" );
    //   MIBY_CLEAR_MISSING_DATA(&m);
    // }
  }
}

void panic() {
  for(int i=0; i < NUMBER_OF_VOICES; i++) {
    voicess[i].gate = 0;
    voicess[i].vca_envelope.value = 0;
    voicess[i].vcf_envelope.value = 0;
    voicess[i].lfo.value = 0;

    voicess[i].dacValues[CV] =          CV_MAX_VALUE / 2;
    voicess[i].dacValues[MOD_AMT] =     MOD_AMT_MIN_VALUE;
    voicess[i].dacValues[WAVE_SELECT] = WAVE_SELECT_ZERO_VALUE;
    voicess[i].dacValues[PWM] =         PWM_MIN_VALUE;
    voicess[i].dacValues[MIXER] =       MIXER_ZERO_VALUE;
    voicess[i].dacValues[RESONANCE] =   RESONANCE_MIN_VALUE;
    voicess[i].dacValues[CUTOFF] =      CUTOFF_ZERO_VALUE;
    voicess[i].dacValues[VCA] =         VCA_MIN_VALUE; //2500; //
  }
}

void updatePitch(uint16_t pitch) {

  uint16_t gain =
             pitch > CV_GAIN_SWITCH_POINT ? CV_GAIN_HIGH : CV_GAIN_LOW;
  uint16_t offset =
             pitch > CV_GAIN_SWITCH_POINT ? (pitch - CV_GAIN_SWITCH_POINT) : pitch;

  // Serial.print("CV: ");Serial.print(pitch);

  updateDAC(gain, constrain(offset,
                            CV_MIN_VALUE,
                            CV_MAX_VALUE >> 1));
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
                                          CUTOFF_MIN_VALUE,
                                          CUTOFF_MAX_VALUE - CUTOFF_GAIN_THRESHOLD));
  } else {
    updateDAC(CUTOFF_GAIN_LOW, constrain(cutoff,
                                         CUTOFF_MIN_VALUE,
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

#define TAB     "\t\t"
#define NEWTAB    "\n" TAB

/*****************************************************************************/
/** Note On                                                                 **/
/*****************************************************************************/
void miby_note_on( miby_this_t midi_state )
{
  Serial.print(MIBY_STATUSBYTE(midi_state));
  Serial.print(MIBY_ARG0(midi_state));
  Serial.println(MIBY_ARG1(midi_state));

  // printf( "<Ch.%d> ", MIBY_CHAN(midi_state));
  // printf( "Note On :: note = %02X, vel = %02X\n", MIBY_ARG0(midi_state),
  //                                                 MIBY_ARG1(midi_state));

  uint8_t voiceNumber = findIdleVoice();
  if(voiceNumber == 0xFF)
    return;
  else {
    voiceNoteOn(voiceNumber,
                MIBY_ARG0(midi_state),
                MIBY_ARG1(midi_state));
  }
}

/*****************************************************************************/
/** Note Off                                                                **/
/*****************************************************************************/
void miby_note_off( miby_this_t midi_state )
{
  Serial.print(MIBY_STATUSBYTE(midi_state));
  Serial.print(MIBY_ARG0(midi_state));
  Serial.println(MIBY_ARG1(midi_state));

  // printf( "<Ch.%d> ", MIBY_CHAN(midi_state) );
  // printf( "Note Off :: note = %02X, vel = %02X\n", MIBY_ARG0(midi_state),
  //                                                  MIBY_ARG1(midi_state) );

  uint8_t voiceNumber = findVoiceWithNote(MIBY_ARG0(midi_state));
  if(voiceNumber == 0xFF)
    return;
  else {
    voiceNoteOff(voiceNumber,
                MIBY_ARG0(midi_state),
                MIBY_ARG1(midi_state));
  }

}

/*****************************************************************************/
/** Realtime System Reset                                                   **/
/*****************************************************************************/
void miby_rt_system_reset( miby_this_t sss )
{
  // printf( NEWTAB "[FF] " );
  // printf( "SysRT: system reset\n" );
  panic();
}

/*****************************************************************************/
/** Realtime Timing Clock                                                   **/
/*****************************************************************************/
void test_rt_timing_clock( miby_this_t sss )
{
  printf( NEWTAB "[F8] " );
  printf( "SysRT: timing clock\n" );
}

/*****************************************************************************/
/** Realtime Start                                                          **/
/*****************************************************************************/
void test_rt_start( miby_this_t sss )
{
  printf( NEWTAB "[FA] " );
  printf( "SysRT: start\n" );
}

/*****************************************************************************/
/** Realtime Continue                                                       **/
/*****************************************************************************/
void test_rt_continue( miby_this_t sss )
{
  printf( NEWTAB "[FB] " );
  printf( "SysRT: continue\n" );
}

/*****************************************************************************/
/** Realtime Stop                                                           **/
/*****************************************************************************/
void test_rt_stop( miby_this_t sss )
{
  printf( NEWTAB "[FC] " );
  printf( "SysRT: stop\n" );
}

/*****************************************************************************/
/** Realtime Active Sense                                                   **/
/*****************************************************************************/
void test_rt_active_sense( miby_this_t sss )
{
  printf( NEWTAB "[FE] " );
  printf( "SysRT: active sense\n" );
}

/*****************************************************************************/
/** MIDI Time Code                                                          **/
/*****************************************************************************/
void test_mtc( miby_this_t sss )
{
  printf( NEWTAB "[%02X %02X] ", MIBY_STATUSBYTE(sss), MIBY_ARG0(sss) );
  printf( "SysCom: Timecode :: type = %02X, value = %02X\n", MIBY_ARG0(sss) >> 4, MIBY_ARG0(sss) & 0xF );
}

/*****************************************************************************/
/** Song Position                                                           **/
/*****************************************************************************/
void test_songpos( miby_this_t sss )
{
  printf( NEWTAB "[%02X %02X %02X] ", MIBY_STATUSBYTE(sss), MIBY_ARG0(sss), MIBY_ARG1(sss) );
  printf( "SysCom: Song Position :: LSB = %02X, MSB = %02X\n", MIBY_ARG0(sss), MIBY_ARG1(sss) );
}

/*****************************************************************************/
/** Song Select                                                             **/
/*****************************************************************************/
void test_songsel( miby_this_t sss )
{
  printf( NEWTAB "[%02X %02X] ", MIBY_STATUSBYTE(sss), MIBY_ARG0(sss) );
  printf( "SysCom: Song Select :: song %02X\n", MIBY_ARG0(sss) );
}

/*****************************************************************************/
/** Tune Request                                                            **/
/*****************************************************************************/
void test_tunereq( miby_this_t sss )
{
  printf( NEWTAB "[%02X] ", MIBY_STATUSBYTE(sss) );
  printf( "SysCom: Tune Request\n" );
}

/*****************************************************************************/
/** Polyphonic Aftertouch                                                   **/
/*****************************************************************************/
void test_poly_at( miby_this_t sss )
{
  printf( NEWTAB "[%02X %02X %02X] ", MIBY_STATUSBYTE(sss), MIBY_ARG0(sss), MIBY_ARG1(sss) );
  printf( "<Ch.%d> ", MIBY_CHAN(sss) );
  printf( "Poly Touch :: note = %02X, pressure = %02X\n", MIBY_ARG0(sss), MIBY_ARG1(sss) );
}

/*****************************************************************************/
/** Continuous Controller                                                   **/
/*****************************************************************************/
void test_cc( miby_this_t sss )
{
  printf( NEWTAB "[%02X %02X %02X] ", MIBY_STATUSBYTE(sss), MIBY_ARG0(sss), MIBY_ARG1(sss) );
  printf( "<Ch.%d> ", MIBY_CHAN(sss) );
  printf( "Ctrl Change :: control # = %02X, value = %02X\n", MIBY_ARG0(sss), MIBY_ARG1(sss) );
}

/*****************************************************************************/
/** Program Change                                                          **/
/*****************************************************************************/
void test_pc( miby_this_t sss )
{
  printf( NEWTAB "[%02X %02X] ", MIBY_STATUSBYTE(sss), MIBY_ARG0(sss) );
  printf( "<Ch.%d> ", MIBY_CHAN(sss) );
  printf( "Prog Change :: program # = %02X\n", MIBY_ARG0(sss) );
}

/*****************************************************************************/
/** Channel Aftertouch                                                      **/
/*****************************************************************************/
void test_chanat( miby_this_t sss )
{
  printf( NEWTAB "[%02X %02X] ", MIBY_STATUSBYTE(sss), MIBY_ARG0(sss) );
  printf( "<Ch.%d> ", MIBY_CHAN(sss) );
  printf( "Chan Touch :: pressure = %02X\n", MIBY_ARG0(sss) );
}

/*****************************************************************************/
/** PitchBend                                                               **/
/*****************************************************************************/
void test_pb( miby_this_t sss )
{
  printf( NEWTAB "[%02X %02X %02X] ", MIBY_STATUSBYTE(sss), MIBY_ARG0(sss), MIBY_ARG1(sss) );
  printf( "<Ch.%d> ", MIBY_CHAN(sss) );
  printf( "Pitchbend :: lsb = %02X, msb = %02X (%04X)\n",
          MIBY_ARG0(sss), MIBY_ARG1(sss),
          ( MIBY_ARG1(sss) << 7 ) + MIBY_ARG0(sss) );
}

/*****************************************************************************/
/** System Exclusive                                                        **/
/*****************************************************************************/
void test_sysex( miby_this_t sss )
{
  int i;
  static char *sysex_state[] = { "IDLE", "START", "MID", "END", "ABORT" };

  printf( NEWTAB "[F0] SYSEX chunk: state = %02X (%s), length = %02X bytes\n",
        MIBY_SYSEX_STATE(sss),
        sysex_state[MIBY_SYSEX_STATE(sss)],
        MIBY_SYSEX_LEN(sss) );
  printf( TAB "\t[" );
  for ( i = 0; i < MIBY_SYSEX_LEN(sss); i++ )
  {
    if ( i )
      printf( " " );
    printf( "%02X", MIBY_SYSEX_BUF(sss,i) );
    if ( i % 8 == 7 )
      printf( NEWTAB "\t" );
  }
  printf( "]" );

  MIBY_SYSEX_DONE_OK(sss);
}

void processSerialInput(char rx_byte) {
  switch(rx_byte){
    case 49:
    case 50:
    case 51:
    case 52:
    case 53:
    case 54: {
        selectedVoice = rx_byte - 49;
        voicess[selectedVoice].gate == 1 ? voiceNoteOff(selectedVoice, 49, 127) : voiceNoteOn(selectedVoice, 49, 0);
      }
      break;
    case 113: {
      if(voicess[selectedVoice].dacValues[CV] + 72 >= CV_MAX_VALUE)
        voicess[selectedVoice].dacValues[CV] = CV_MAX_VALUE;
      else
        voicess[selectedVoice].dacValues[CV] = voicess[selectedVoice].dacValues[CV] + 72;
    }
    break;
    case 97: {
      if(voicess[selectedVoice].dacValues[CV] < 72)
        voicess[selectedVoice].dacValues[CV] = 0;
      else
        voicess[selectedVoice].dacValues[CV] = voicess[selectedVoice].dacValues[CV] - 72;
    }
    break;
    case 119: {
      if(voicess[selectedVoice].dacValues[MOD_AMT] + 25 >= MOD_AMT_MAX_VALUE)
        voicess[selectedVoice].dacValues[MOD_AMT] = MOD_AMT_MAX_VALUE;
      else
        voicess[selectedVoice].dacValues[MOD_AMT] = voicess[selectedVoice].dacValues[MOD_AMT] + 25;
    }
    break;
    case 115: {
      if(voicess[selectedVoice].dacValues[MOD_AMT] < 25)
        voicess[selectedVoice].dacValues[MOD_AMT] = 0;
      else
      voicess[selectedVoice].dacValues[MOD_AMT] = voicess[selectedVoice].dacValues[MOD_AMT] - 25;
    }
    break;
    case 101: {
      if(voicess[selectedVoice].dacValues[WAVE_SELECT] + 25 >= WAVE_SELECT_MAX_VALUE)
        voicess[selectedVoice].dacValues[WAVE_SELECT] = WAVE_SELECT_MAX_VALUE;
      else
        voicess[selectedVoice].dacValues[WAVE_SELECT] = voicess[selectedVoice].dacValues[WAVE_SELECT] + 25;
    }
    break;
    case 100: {
      if(voicess[selectedVoice].dacValues[WAVE_SELECT] < 25)
        voicess[selectedVoice].dacValues[WAVE_SELECT] = 0;
      else
        voicess[selectedVoice].dacValues[WAVE_SELECT] = voicess[selectedVoice].dacValues[WAVE_SELECT] - 25;
      }
    break;

    case 114: {
      if(voicess[selectedVoice].dacValues[PWM] + 25 >= PWM_MAX_VALUE)
        voicess[selectedVoice].dacValues[PWM] = PWM_MAX_VALUE;
      else
        voicess[selectedVoice].dacValues[PWM] = voicess[selectedVoice].dacValues[PWM] + 25;
    }
    break;
    case 102: {
      if(voicess[selectedVoice].dacValues[PWM] < PWM_MIN_VALUE + 25)
        voicess[selectedVoice].dacValues[PWM] = PWM_MIN_VALUE;
      else
        voicess[selectedVoice].dacValues[PWM] = voicess[selectedVoice].dacValues[PWM] - 25;
      }
      break;

    case 116: {
      if(voicess[selectedVoice].dacValues[MIXER] + 25 >= MIXER_MAX_VALUE)
        voicess[selectedVoice].dacValues[MIXER] = MIXER_MAX_VALUE;
      else
        voicess[selectedVoice].dacValues[MIXER] = voicess[selectedVoice].dacValues[MIXER] + 25;
    }
    break;
    case 103: {
      if(voicess[selectedVoice].dacValues[MIXER] < 25)
        voicess[selectedVoice].dacValues[MIXER] = 0;
      else
        voicess[selectedVoice].dacValues[MIXER] = voicess[selectedVoice].dacValues[MIXER] - 25;
      }
      break;

    case 121: {
      if(voicess[selectedVoice].dacValues[CUTOFF] + 25 >= CUTOFF_MAX_VALUE)
        voicess[selectedVoice].dacValues[CUTOFF] = CUTOFF_MAX_VALUE;
      else
        voicess[selectedVoice].dacValues[CUTOFF] = voicess[selectedVoice].dacValues[CUTOFF] + 25;
      }
      break;
    case 104: {
      if(voicess[selectedVoice].dacValues[CUTOFF] < 25)
        voicess[selectedVoice].dacValues[CUTOFF] = 0;
      else
        voicess[selectedVoice].dacValues[CUTOFF] = voicess[selectedVoice].dacValues[CUTOFF] - 25;
      }
      break;

    case 117: {
      if(voicess[selectedVoice].dacValues[RESONANCE] + 25 >= RESONANCE_MAX_VALUE)
        voicess[selectedVoice].dacValues[RESONANCE] = RESONANCE_MAX_VALUE;
      else
        voicess[selectedVoice].dacValues[RESONANCE] = voicess[selectedVoice].dacValues[RESONANCE] + 25;
      }
      break;
    case 106: {
      if(voicess[selectedVoice].dacValues[RESONANCE] < 25)
        voicess[selectedVoice].dacValues[RESONANCE] = 0;
      else
        voicess[selectedVoice].dacValues[RESONANCE] = voicess[selectedVoice].dacValues[RESONANCE] - 25;
      }
      break;

    case 105: {
      if(voicess[selectedVoice].dacValues[VCA] + 25 >= VCA_MAX_VALUE)
        voicess[selectedVoice].dacValues[VCA] = VCA_MAX_VALUE;
      else
        voicess[selectedVoice].dacValues[VCA] = voicess[selectedVoice].dacValues[VCA] + 25;
      }
      break;
    case 107: {
      if(voicess[selectedVoice].dacValues[VCA] < 25)
        voicess[selectedVoice].dacValues[VCA] = 0;
      else
        voicess[selectedVoice].dacValues[VCA] = voicess[selectedVoice].dacValues[VCA] - 25;
      }
      break;

    case 122: {
        panic(); // zero DAC
      }
      break;
  }
  printVoiceDacValues(&voicess[0]);
  printVoiceDacValues(&voicess[1]);
  printVoiceDacValues(&voicess[2]);
  printVoiceDacValues(&voicess[3]);
  printVoiceDacValues(&voicess[4]);
  printVoiceDacValues(&voicess[5]);
  Serial.println("------------------------------");
}
