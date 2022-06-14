#include <SPI.h>
#include <DirectIO.h>
#include "miby.h"

// #define DISPLAY_MIDI_DATA
// #define DISPLAY_VOICE_DATA
// #define DISPLAY_DAC_DATA
#define DISPLAY_AUTOTUNE_INFO
#define USE_KEYBOARD

// #define DISABLE_ENVELOPES
// #define DAC_TEST
#define DISABLE_AUTOTUNE

#define VREF 4096
#define DAC_STEPS 4095
#define VOLT_PER_OCTAVE_SEMITONE 750/12 // 0.750V/12 => 0.0625V => 62.5 DAC steps between two consecutive half note
#define DAC_STEP_PER_SEMITONE (VOLT_PER_OCTAVE_SEMITONE) / (VREF / DAC_STEPS)
// (0.750 / 12) / (4.096 / 4095) = 62.4847412109

#define NUMBER_OF_VOICES 6
#define NUMBER_OF_PARAMS 8

#define AUTOTUNE_STEP_SIZE 3
#define AUTOTUNE_WIDTH 25

#ifdef DAC_TEST

uint16_t gain = 0;
uint16_t offset = 0;

#define DAC_MAX_CODE 4095
#define DAC_TEST_STEP_SIZE 127

#endif // DAC_TEST

enum DAC_PARAM {
  DAC_PARAM_CUTOFF,
  DAC_PARAM_RESONANCE,
  DAC_PARAM_MIXER,
  DAC_PARAM_VCA,
  DAC_PARAM_PWM,
  DAC_PARAM_CV,
  DAC_PARAM_WAVE_SELECT,
  DAC_PARAM_MOD_AMT,
  DAC_PARAM_LAST
};

// GOOD default settings:
// Pitch: 5336
// MOD: 0
// WAVE: 5500 (SAW)
// PWM: 0
// MIX: 3550
// VCF: 500
// RES: 0
// VCA: 2975

#define CUTOFF                0     // Order in multiplexer
#define CUTOFF_GAIN_LOW       1600  // Where to set the OPAMPs gain when the CV is below 0V
#define CUTOFF_GAIN_THRESHOLD 1600  // After which DAC value to switch over to the other gain
#define CUTOFF_GAIN_HIGH      0     // Where to set the OPAMPs gain when the CV is above 0V
#define CUTOFF_MIN_VALUE      0     // The smallest DAC value for this CV
#define CUTOFF_MAX_VALUE      5600  // The largest DAC value for this CV
#define CUTOFF_ZERO_VALUE     500   // Where to reset this CV value
// -3 .. +4 V
// -3/8V per octave => -0.375V per octave
// -1.6V  24kHz => 4V 0 Hz (Inverted)

#define RESONANCE           1       // Order in multiplexer
#define RESONANCE_GAIN      0       // Where to set the OPAMPs gain for this CV
#define RESONANCE_MIN_VALUE 0       // The smallest DAC value for this CV
#define RESONANCE_MAX_VALUE 2500    // The largest DAC value for this CV
// 0 .. +2.5 V

#define MIXER            2          // Order in multiplexer
#define MIXER_GAIN       2000       // Where to set the OPAMPs gain for this CV
#define MIXER_MIN_VALUE  0          // The smallest DAC value for this CV VCA2 FULL VCA1 OFF
#define MIXER_MAX_VALUE  4000       // The largest DAC value for this CV  VCA2 OFF  VCA1 FULL
#define MIXER_VCO_MAX_VALUE  3000   // The DAC value where the VCO input is dominant
#define MIXER_NOSIE_MAX_VALUE  1000 // The DAC value where the noise input is dominant
#define MIXER_ZERO_VALUE 2000       // Where to reset this CV value       VCA2 -6dB VCA1 -6dB
// -2 .. 2 V

#define VCA            3            // Order in multiplexer
#define VCA_GAIN       0            // Where to set the OPAMPs gain for this CV
#define VCA_MIN_VALUE  0            // The smallest DAC value for this CV
#define VCA_MAX_VALUE  4200         // The largest DAC value for this CV
#define VCA_AUTOTUNE_VALUE 3000     // VCA DAC value for autotune
#define VCA_ZERO_VALUE 0            // The default DAC value for this CV
// 0 .. +4.3 V

#define PWM            4            // Order in multiplexer
#define PWM_GAIN       0            // Where to set the OPAMPs gain for this CV
#define PWM_MIN_VALUE  0            // The smallest DAC value for this CV
#define PWM_MAX_VALUE  2200         // The largest DAC value for this CV
#define PWM_ZERO_VALUE 1100         // The default DAC value for this CV
// 0 .. +2.2 V
// Needs to be/Can be set to a slightly negative CV to stop the SAW PWM

#define CV                    5     // Order in multiplexer
#define CV_GAIN_LOW           4095  // Where to set the OPAMPs gain when the CV is below 0V
#define CV_GAIN_SWITCH_POINT  4096  // After which DAC value to switch over to the other gain
#define CV_GAIN_HIGH          0     // Where to set the OPAMPs gain when the CV is above 0V
#define CV_MIN_VALUE          0     // The smallest DAC value for this CV
#define CV_MAX_VALUE          8192  // The largest DAC value for this CV
#define CV_ZERO_VALUE         4095  // Where to reset this CV value
#define CV_AUTOTUNE_VALUE     3750  // Where to reset this CV value
// -4 .. +4 V
// 3/4V per octave => 0.750mV per octave
// 0.750V/12 => 0.0625V => 62.5 DAC steps between two consecutive half note

#define WAVE_SELECT             6     // Order in multiplexer
#define WAVE_SELECT_GAIN        2000  // Where to set the OPAMPs gain for this CV (-1V)
#define WAVE_SELECT_MIN_VALUE   0     // The smallest DAC value for this CV
#define WAVE_SELECT_MAX_VALUE   6000  // The largest DAC value for this CV
#define WAVE_SELECT_ZERO_VALUE  700   // Where to reset this CV value
#define WAVE_SELECT_SQR         1000  // (-2 + 1)   -1V   SQR
#define WAVE_SELECT_TRI         2700  // (-2 + 2.7) +0.7V TRI
#define WAVE_SELECT_TRI_SAW     4000  // (-2 + 4)   +2V   TRI_SAW
#define WAVE_SELECT_SAW         5500  // (-2 + 5.5) +3.5V SAW
// -2 .. +4 V
// -2 V <-SQR-> -0.3 (+/-0.15) V <-TRI-> +1.25 (+/-0.3) V <-TRI + SAW-> +2.7 (+/-0.5) V <-SAW-> 4V

#define MOD_AMT           7           // Order in multiplexer
#define MOD_AMT_GAIN      0           // Where to set the OPAMPs gain for this CV
#define MOD_AMT_MIN_VALUE 0           // The smallest DAC value for this CV
#define MOD_AMT_MAX_VALUE 4000        // The largest DAC value for this CV
// 0 .. +4 V

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

#define ENV_IDLE    0
#define ENV_ATTACK  1
#define ENV_DECAY   2
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

enum VCO_SHAPE {
  VCO_SHAPE_SQR,
  VCO_SHAPE_TRI,
  VCO_SHAPE_TRI_SAW,
  VCO_SHAPE_SAW,
  VCO_SHAPE_LAST
};

typedef struct voice_structure {
  envelope_structure vca_envelope = {
    ENV_IDLE,          // state
    VCA_MIN_VALUE,     // min_value
    VCA_MAX_VALUE,     // max_value
    VCA_MIN_VALUE,     // value 0 - 3500
    10,                // attack_rate
    30,                // decay_rate
    VCA_MAX_VALUE,     // sustain_value
    10                 // release_rate
  };
  envelope_structure vcf_envelope = {
    ENV_IDLE,
    CUTOFF_MIN_VALUE,  // min_value
    CUTOFF_MAX_VALUE,  // max_value
    CUTOFF_MIN_VALUE,  // value 0 - 6000
    60,                // attack_rate
    10,                // decay_rate
    CUTOFF_ZERO_VALUE, // sustain_value
    60                 // release_rate
  };
  lfo_structure lfo = {
    LFO_TRI,           // shape
    LFO_PHASE_UP,      // phase
    1,                 // rate
    LFO_MIN_VALUE,     // min_value
    LFO_MAX_VALUE,     // max_value
    LFO_MIN_VALUE,     // accumulator
    LFO_MIN_VALUE      // value
  };
  uint16_t dacValues[8] = {
    CUTOFF_ZERO_VALUE,
    RESONANCE_MIN_VALUE,
    MIXER_MAX_VALUE,
    VCA_ZERO_VALUE,
    PWM_ZERO_VALUE,
    CV_ZERO_VALUE,
    WAVE_SELECT_SQR,
    MOD_AMT_MIN_VALUE
  };
  uint8_t gate = 0;
  uint8_t note = 0;
  uint8_t velocity = 0;
  uint8_t vco_shape = VCO_SHAPE_SQR;
  uint16_t square_pwm = PWM_ZERO_VALUE;
  uint16_t other_pwm = PWM_MIN_VALUE;
  uint16_t frequency_at_zero_volt = 0; // Simple frequency error at 0V
  int dac_offset = 0;
} voice;

voice voicess[6];

uint16_t vco_shapes[4] = {
  WAVE_SELECT_SQR,
  WAVE_SELECT_TRI,
  WAVE_SELECT_TRI_SAW,
  WAVE_SELECT_SAW
};

void updatePitch(voice *voice);
void updateModAmount(voice *voice);
void updateWaveSelect(voice *voice);
void updatePwm(voice *voice);
void updateMixer(voice *voice);
void updateResonance(voice *voice);
void updateCutoff(voice *voice);
void updateVca(voice *voice);

void (*updateVoiceParam[8])(voice *) = {
  updateCutoff,
  updateResonance,
  updateMixer,
  updateVca,
  updatePwm,
  updatePitch,
  updateWaveSelect,
  updateModAmount
};

uint16_t exp_vcf_lookup[128] = {
  6532, 6095, 5688, 5308, 4953, 4622, 4313, 4025, 3756, 3505, 3270, 3052, 2848,
  2657, 2480, 2314, 2159, 2015, 1880, 1755, 1637, 1528, 1426, 1330, 1241, 1158,
  1081, 1009, 941, 878, 820, 765, 714, 666, 621, 580, 541, 505, 471, 440, 410,
  383, 357, 333, 311, 290, 271, 253, 236, 220, 205, 191, 179, 167, 155, 145, 135,
  126, 118, 110, 102, 96, 89, 83, 78, 72, 67, 63, 59, 55, 51, 48, 44, 41, 39, 36,
  34, 31, 29, 27, 25, 24, 22, 20, 19, 18, 17, 15, 14, 13, 12, 12, 11, 10, 9, 9, 8,
  7, 7, 6, 6, 6, 5, 5, 4, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1
};

uint16_t exp_vca_lookup[128] = {
  4027, 3773, 3534, 3310, 3101, 2905, 2721, 2549, 2387, 2236, 2095, 1962, 1838,
  1722, 1613, 1511, 1415, 1325, 1242, 1163, 1089, 1020, 956, 895, 839, 785, 736,
  689, 646, 605, 566, 531, 497, 465, 436, 408, 382, 358, 336, 314, 294, 276, 258,
  242, 227, 212, 199, 186, 174, 163, 153, 143, 134, 126, 118, 110, 103, 97, 90,
  85, 79, 74, 70, 65, 61, 57, 53, 50, 47, 44, 41, 38, 36, 34, 31, 29, 28, 26, 24,
  23, 21, 20, 18, 17, 16, 15, 14, 13, 12, 11, 11, 10, 9, 9, 8, 8, 7, 7, 6, 6, 5,
  5, 5, 4, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

// Values to map from CC
uint8_t controlValues[32];

  // VCO frequency
  uint8_t coarse;           // CC 2
                            // octave (62.5 x 12) = 750

  uint8_t fine;             // CC 3
                            // 62.5 per semitone
                            // 0-127 => +/- semitone
                            // DAC value += fine

  uint8_t glide;            // CC 4

  uint8_t lfoToVco;         // CC 5

  uint8_t envToVco;         // CC 6
                            // -/+ ?

  //wave shape
  uint8_t waveshape;        // CC 7
                            // 0..127 mapped to 0..4000 => 4000/127 = 31.xxx
                            // DAC value = waveshape << 5

  uint8_t pwm;              // CC 8
                            // 0..127 mapped to 0..2000 => 2000/127 = 15.xxx
                            // DAC value += pwm << 4

  uint8_t lfoToPwm;         // CC 9

  uint8_t envToPwm;         // CC 10
                            // -/+ ?
  // mixer
  uint8_t noise;            // CC 11
                            // 0..127 mapped to 0..3000 => 3000/127 = 23.6 ~ 16
                            // DAC value = 500 + (noise << 4)

  uint8_t lfoToMixer;       // CC 12

  uint8_t envToMixer;       // CC 13
                            // -/+ ?

  // filter
  uint8_t cutoff;           // CC 14
                            // 0..127 mapped to 0..6000 => 6000/127 = 47.x
                            // 32*127+16*127 - 127 = 5969
                            // DAC value = (cutoff << 5) + (cutoff << 4) - cutoff;

  uint8_t resonance;        // CC 15
                            // 0..127 mapped to 0..2000 => 2000/127 = 15.xxx
                            // DAC value = resonance << 4

  uint8_t filterAttack;     // CC 16
                            // 0..127 mapped to 6000..1 exponentially
                            // vcf_envelope.attack_rate = exp_vcf_lookup[127 - filterAttack]

  uint8_t filterDecay;      // CC 17
                            // 0..127 mapped to ?..1
                            // vcf_envelope.decay_rate = exp_vcf_lookup[127 - filterDecay]

  uint8_t filterSustain;    // CC 18
                            // 0..127 mapped to 6000..1 linear (cutoff is inverted)
                            // vcf_envelope.sustain_value = CUTOFF_MAX_VALUE - ((filterSustain << 5) + (filterSustain << 4) - filterSustain);

  uint8_t filterRelease;    // CC 19
                            // 0..127 mapped to 6000..1
                            // vcf_envelope.release_rate = exp_vcf_lookup[127 - filterDecay]

  uint8_t modAmount;        // CC 20
                            // 0..127 mapped to 0..3500 => 3500/127 = 27.5 = 16+8+2+1
                            // 16*127+8*127+2*127+127 = 3429
                            // DAC value = (modAmount << 4) + (modAmount << 3) + (modAmount << 1) + modAmount

  uint8_t keyboardTrack;    // CC 21
                            // amount or [off/half/full/reversed]?
                            // needs a table?
                            // convert cutoff scale to 8 octave
                            // cutoff = cutoff + lfo +/- env + note

  uint8_t lfoToCutoff;      // CC 22
  uint8_t envToCutoff;      // CC 23

  // VCA
  uint8_t ampAttack;        // CC 24
                            // 0..127 mapped to 3500..1 exponentially
                            // vca_envelope.attack_rate = exp_vca_lookup[127 - ampAttack]

  uint8_t ampDecay;         // CC 25
                            // 0..127 mapped to 3500..1 exponentially
                            // vca_envelope.decay_rate = exp_vca_lookup[127 - ampDecay]

  uint8_t ampSustain;       // CC 26
                            // 0..127 mapped to 1..3500 (see modAmount)
                            // vca_envelope.sustain_value = (ampSustain << 4) + (ampSustain << 3) + (ampSustain << 1) + ampSustain

  uint8_t ampRelease;       // CC 27
                            // 0..127 mapped to 1..3500 exponentially
                            // vca_envelope.release_rate = exp_vca_lookup[127 - ampRelease]

  uint8_t lfoToVca;         // CC 28
  uint8_t envToVca;         // CC 29
                            // -/+ ?

#define CC_MODWHEEL         0
#define CC_COARSE           1
#define CC_FINE             2
#define CC_GLIDE_            3
#define CC_LFOTOVCO_         4
#define CC_ENVTOVCO_         5
#define CC_WAVESHAPE        6
#define CC_PWM              7
#define CC_LFOTOPWM_         8
#define CC_ENVTOPWM_         9
#define CC_NOISE            10
#define CC_LFOTOMIXER_       11
#define CC_ENVTOMIXER_       12
#define CC_CUTOFF           13
#define CC_RESONANCE        14
#define CC_FILTERATTACK     15
#define CC_FILTERDECAY      16
#define CC_FILTERSUSTAIN    17
#define CC_FILTERRELEASE    18
#define CC_MODAMOUNT        19
#define CC_KEYBOARDTRACK_    20
#define CC_LFOTOCUTOFF_      21
#define CC_ENVTOCUTOFF_      22
#define CC_AMPATTACK        23
#define CC_AMPDECAY         24
#define CC_AMPSUSTAIN       25
#define CC_AMPRELEASE       26
#define CC_LFOTOVCA_         27
#define CC_ENVTOVCA_         28
#define CC_RESERVED_1_       29
#define CC_RESERVED_2_       30
#define CC_RESERVED_3_       31

#define MIBY_SYSEX_BUF_LEN  ( 32 )

// extern void test_rt_timing_clock( miby_this_t );
#define MIBY_HND_RT_CLOCK NULL // test_rt_timing_clock

// extern void test_rt_start( miby_this_t );
#define MIBY_HND_RT_START NULL // test_rt_start

// extern void test_rt_continue( miby_this_t );
#define MIBY_HND_RT_CONTINUE NULL //  test_rt_continue

// extern void test_rt_stop( miby_this_t );
#define MIBY_HND_RT_STOP NULL // test_rt_stop

// extern void test_rt_active_sense( miby_this_t );
#define MIBY_HND_RT_ACT_SENSE NULL // test_rt_active_sense

// extern void miby_rt_system_reset( miby_this_t );
#define MIBY_HND_RT_SYS_RESET NULL // miby_rt_system_reset

// extern void test_tunereq( miby_this_t );
#define MIBY_HND_SYS_TIMEREQ  NULL // test_tunereq

// extern void test_mtc( miby_this_t );
#define MIBY_HND_SYS_MTC NULL // test_mtc

// extern void test_songpos( miby_this_t );
#define MIBY_HND_SYS_SONGPOS NULL // test_songpos

// extern void test_songsel( miby_this_t );
#define MIBY_HND_SYS_SONGSEL NULL // test_songsel

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

#ifdef DISPLAY_DAC_DATA
void printVoiceDacValues(voice *voice) {
  Serial.print("Gate: ");Serial.print(voice->gate);
  Serial.print(" PCV: ");Serial.print(voice->dacValues[CV]);
  Serial.print(" OFS: ");Serial.print(voice->dac_offset);
  Serial.print(" MOD: ");Serial.print(voice->dacValues[MOD_AMT]);
  Serial.print(" WAV: ");Serial.print(voice->dacValues[WAVE_SELECT]);
  Serial.print(" PWM: ");Serial.print(voice->dacValues[PWM]);
  Serial.print(" MIX: ");Serial.print(voice->dacValues[MIXER]);
  Serial.print(" VCF: ");Serial.print(voice->dacValues[CUTOFF]);
  Serial.print(" RES: ");Serial.print(voice->dacValues[RESONANCE]);
  Serial.print(" VCA: ");Serial.println(voice->dacValues[VCA]);
}
#endif DISPLAY_DAC_DATA
#ifdef DISPLAY_VOICE_DATA
void printEnvelopeParams(voice *voice) {
  Serial.print("VCA A: ");Serial.print(voice->vca_envelope.attack_rate);
  Serial.print(" D: ");Serial.print(voice->vca_envelope.decay_rate);
  Serial.print(" S: ");Serial.print(voice->vca_envelope.sustain_value);
  Serial.print(" R: ");Serial.print(voice->vca_envelope.release_rate);
  Serial.print(" STATE: ");Serial.print(voice->vca_envelope.state);
  Serial.print(" VALUE: ");Serial.print(voice->vca_envelope.value);
  Serial.print(" MIN: ");Serial.print(voice->vca_envelope.min_value);
  Serial.print(" MAX: ");Serial.println(voice->vca_envelope.max_value);

  // Serial.print(" VCF A: ");Serial.print(voice->vcf_envelope.attack_rate);
  // Serial.print(" D: ");Serial.print(voice->vcf_envelope.decay_rate);
  // Serial.print(" S: ");Serial.print(voice->vcf_envelope.sustain_value);
  // Serial.print(" R: ");Serial.print(voice->vcf_envelope.release_rate);
  // Serial.print(" STATE: ");Serial.print(voice->vcf_envelope.state);
  // Serial.print(" VALUE: ");Serial.println(voice->vcf_envelope.value);

  // Serial.print("LFO_SHAPE: ");Serial.print(voice->lfo.shape);
  // Serial.print(" LFO_ACC: ");Serial.print(voice->lfo.accumulator);
  // Serial.print(" LFO_VALUE: ");Serial.println(voice->lfo.value);
}
#endif // DISPLAY_VOICE_DATA

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
        envelope->state = ENV_RELEASE;
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
      if(envelope->min_value + envelope->release_rate >= envelope->value){
        envelope->value = envelope->min_value;
        envelope->state = ENV_IDLE;
      }
      else
        envelope->value -= envelope->release_rate;
      break;
    default:
      envelope->value = envelope->min_value;
  }
}

#define NOTE_OFF 0
#define NOTE_ON  1

static constexpr unsigned k_pinChipSelectDAC = 7;

OutputPort<PORT_B> voiceParamSelect;                    // Arduino pin 8, 9, 10 to 4051 11, 10, 9
OutputPort<PORT_D, 2, 5> voiceSelectPort;               // Arduino pin 3, 4, 5, 6
Output<k_pinChipSelectDAC>  m_outputChipSelectDAC;      // Arduino pin 7 to MP4922 pin 3

#define DAC_A_GAIN 1 // 0: x2, 1: x1
#define DAC_B_GAIN 1 // 0: x2, 1: x1

#define DAC_A_BUFFER 1 // 0: Unbuffered, 1: Buffered
#define DAC_B_BUFFER 1 // 0: Unbuffered, 1: Buffered

#define DAC_A_SHDN 1 // 0: Channel off, 1: Channel on
#define DAC_B_SHDN 1 // 0: Channel off, 1: Channel on

#define k_writeChannelA (((0x0 << 7) | (DAC_A_BUFFER << 6) | (DAC_A_GAIN << 5) | (DAC_A_SHDN << 4)) << 8)
#define k_writeChannelB (((0x1 << 7) | (DAC_B_BUFFER << 6) | (DAC_B_GAIN << 5) | (DAC_B_SHDN << 4)) << 8)

// http://www.8bit-era.cz/arduino-timer-interrupts-calculator.html
// https://www.instructables.com/Arduino-Timer-Interrupts/
// https://timer-interrupt-calculator.simsso.de
// https://forum.arduino.cc/t/uno-timer2-interrupt-formula-to-calculate-frequency/881061/4
void initializeInterrupts() {
  cli();                                    // stop interrupts
  // // 6857 Hz = (1/(7/1000)) * 6 * 8
  // TIMER 1 for interrupt frequency 6858.122588941277 Hz:
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; // initialize counter value to 0
  // set compare match register for 6858.122588941277 Hz increments
  OCR1A = 2332; // = 16000000 / (1 * 6858.122588941277) - 1 (must be <65536)
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
  // OCR2A = 176; // = 16000000 / (64 * 1146.788990825688) - 1 (must be <256)
  OCR2A = 499; // = 16000000 / (64 * 1146.788990825688) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS22, CS21 and CS20 bits for 64 prescaler
  TCCR2B |= (1 << CS22) | (0 << CS21) | (0 << CS20);
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
}

uint8_t preMainLoop = 1;
uint8_t preAutotuneLoop = 1;
uint8_t irq1Count = 0;
uint16_t irq2Count = 0;

uint8_t notVoiceSelectTable[6] = { 0x3E, 0x3D, 0x3B, 0x37, 0x2F, 0x1F };
#define DISABLE_MULTIPLEX 0x3F

ISR(TIMER1_COMPA_vect) {                // interrupt commands for TIMER 1
  if(preAutotuneLoop)
    return;
  uint8_t voiceNumber = 0;
  uint8_t voiceParamNumber = 0;

  voiceSelectPort.write(DISABLE_MULTIPLEX);                     // Disable Voice param select multiplexer

  voiceNumber = irq1Count >> 3;                                 // 0..5 => 0b000xxx ... 0b101xxx
  voiceParamNumber = irq1Count & 0x07;                          // 0..7 => 0bxxx000 ... 0bxxx111

  // if(voicess[voiceNumber].vca_envelope.state != ENV_IDLE) {
  if(voiceNumber < 5) {
    updateVoiceParam[voiceParamNumber](&voicess[voiceNumber]);  // Send value to DAC
    voiceParamSelect.write(voiceParamNumber);                   // VoiceParam Select
    voiceSelectPort.write(notVoiceSelectTable[voiceNumber]);    // Voice Select / Enable voice param select multiplexer
                                                                // We update the voice param 4051 first,
                                                                // because the voice channel select pins
                                                                // (Arduino D3-D4-D5-D6) are functioning as
                                                                // a multiplex enable/disable pin.
                                                                // The voice param select 4051 is disabled
                                                                // till the DAC is updated.
  }

  irq1Count++;
  if(irq1Count > 47) {
    irq1Count = 0;
  }
}

ISR(TIMER2_COMPA_vect) {                 // interrupt commands for TIMER 2 here
  if(preMainLoop)
    return;
  // Update Envelope
  for(int i = 0; i < NUMBER_OF_VOICES; i ++) {
    // if(continuous_controller_changed) {}
    // voicess[i].vcf_envelope.attack_rate = exp_vcf_lookup[controlValues[CC_FILTERATTACK]];
    // voicess[i].vcf_envelope.decay_rate = exp_vcf_lookup[1controlValues[CC_FILTERDECAY]];
    // voicess[i].vcf_envelope.sustain_value = (controlValues[CC_FILTERSUSTAIN] << 5) + (controlValues[CC_FILTERSUSTAIN] << 4) - controlValues[CC_FILTERSUSTAIN];
    // voicess[i].vcf_envelope.release_rate = exp_vcf_lookup[controlValues[CC_FILTERRELEASE]];

    // voicess[i].vca_envelope.attack_rate = exp_vca_lookup[controlValues[CC_AMPATTACK]];
    // voicess[i].vca_envelope.decay_rate = exp_vca_lookup[controlValues[CC_AMPDECAY]];
    // voicess[i].vca_envelope.sustain_value = (controlValues[CC_AMPSUSTAIN] << 4) + (controlValues[CC_AMPSUSTAIN] << 3) + (controlValues[CC_AMPSUSTAIN] << 1) + controlValues[CC_AMPSUSTAIN];
    // voicess[i].vca_envelope.release_rate = exp_vca_lookup[controlValues[CC_AMPRELEASE]];

    updateEnvelope(&voicess[i].vca_envelope, &voicess[i]);
    updateEnvelope(&voicess[i].vcf_envelope, &voicess[i]);

    updateLfo(&voicess[i]);
    // voicess[i].dacValues[PWM] = voicess[i].lfo.value;

    // TODO: sort this out
    // voicess[i].dacValues[WAVE_SELECT] = controlValues[CC_WAVESHAPE] << 5;

    // voicess[i].dacValues[PWM] = controlValues[CC_PWM] << 4;

    // voicess[i].dacValues[MIXER] = 500 + (controlValues[CC_NOISE] << 4);

    // voicess[i].dacValues[CUTOFF] = CUTOFF_MAX_VALUE -
    //                                ((controlValues[CC_CUTOFF] << 5) +
    //                                 (controlValues[CC_CUTOFF] << 4) -
    //                                 controlValues[CC_CUTOFF] +
    //                                 voicess[i].vcf_envelope.value);

    // voicess[i].dacValues[RESONANCE] = (controlValues[CC_RESONANCE] << 4);

    // voicess[i].dacValues[MOD_AMT] = (controlValues[CC_MODAMOUNT] << 4) +
    //                                 (controlValues[CC_MODAMOUNT] << 3) +
    //                                 (controlValues[CC_MODAMOUNT] << 1) +
    //                                  controlValues[CC_MODAMOUNT];

#ifndef DISABLE_ENVELOPES
    voicess[i].dacValues[CUTOFF] = voicess[i].vcf_envelope.value;
    voicess[i].dacValues[VCA]    = voicess[i].vca_envelope.value;
#endif // DISABLE_ENVELOPES
  }

  irq2Count++;
  if(irq2Count > 1146){
    irq2Count = 0;
  }
}

uint32_t lastMessageReceived = 0;

miby_t m;

void setup() {
  cli();                                    // stop interrupts

  m_outputChipSelectDAC = LOW;     // Disable DAC (~CS)

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2); // original

  // Serial.begin(115200);
  Serial.begin(31250); // MIDI?
  Serial.println("Hello world");
#ifndef DAC_TEST
#ifndef USE_KEYBOARD
  miby_init( &m, NULL );
#endif // USE_KEYBOARD
  panic();

  initializeInterrupts();

#ifndef DISABLE_AUTOTUNE
  setupAutotune();

  sei();                                    // allow interrupts

  preAutotuneLoop = 0;

  runAutotune();

  teardownAutotune();
#else // DISABLE_AUTOTUNE
  preAutotuneLoop = 0;
#endif // DISABLE_AUTOTUNE

#endif // DAC_TEST

  sei();                                    // allow interrupts

  preMainLoop = 0;

  lastMessageReceived = millis();
}

void loop() {
#ifdef DAC_TEST
  if(millis() - lastMessageReceived > 5000) {
    lastMessageReceived = millis();

    Serial.print("Gain|Offset: ");
    Serial.print(gain);Serial.print("|");Serial.println(offset);
    updateDAC(constrain(gain, 0, DAC_MAX_CODE),
              constrain(offset, 0, DAC_MAX_CODE));

    offset += DAC_TEST_STEP_SIZE;
    if(offset >= DAC_MAX_CODE){
      offset = 0;
      gain += DAC_TEST_STEP_SIZE;
    }
    if(gain >= DAC_MAX_CODE){
      offset = 0;
      gain = 0;
    }
  }
#else // DAC_TEST

  if (Serial.available() > 0) {    // Midi/Serial input available?
    char rx_byte = 0;
    rx_byte = Serial.read();       // Get midi/serial input

#ifdef USE_KEYBOARD
    Serial.print(rx_byte);
    processSerialInput(rx_byte);
#else // USE_KEYBOARD
    miby_parse( &m, rx_byte);
    if ( MIBY_ERROR_MISSING_DATA(&m) )
    {
      Serial.println( "*** MISSING DATA ***\n" );
      MIBY_CLEAR_MISSING_DATA(&m);
    }
#endif // USE_KEYBOARD
  }

  if(millis() - lastMessageReceived > 1000) {
    lastMessageReceived = millis();
#ifdef DISPLAY_DAC_DATA
    // Serial.println(irq1Count);
    // Serial.println(irq2Count);
    printVoiceDacValues(&voicess[0]);
    printVoiceDacValues(&voicess[1]);
    printVoiceDacValues(&voicess[2]);
    printVoiceDacValues(&voicess[3]);
    // printVoiceDacValues(&voicess[4]);
    // printVoiceDacValues(&voicess[5]);
#endif // DISPLAY_DAC_DATA

#ifdef DISPLAY_VOICE_DATA
    printEnvelopeParams(&voicess[0]);
    // printEnvelopeParams(&voicess[1]);
    // printEnvelopeParams(&voicess[2]);
    // printEnvelopeParams(&voicess[3]);
#endif // DISPLAY_VOICE_DATA

    // Serial.println("------------------------------");
  }
#endif // DAC_TEST
}

#ifdef USE_KEYBOARD

uint8_t selectedVoiceNumber = 0;
voice *selectedVoice;

void processSerialInput(char rx_byte) {
  Serial.println((int)(rx_byte - 49));
  switch(rx_byte){
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
    // PITCH
    case 113: { // q
      selectedVoice->dacValues[CV] = constrain(selectedVoice->dacValues[CV] - AUTOTUNE_STEP_SIZE, //62
                                               CV_MIN_VALUE,
                                               CV_MAX_VALUE);
    }
    break;
    case 97: { // a
      selectedVoice->dacValues[CV] = constrain(selectedVoice->dacValues[CV] + AUTOTUNE_STEP_SIZE, //62
                                               CV_MIN_VALUE,
                                               CV_MAX_VALUE);
    }
    break;
    // MOD
    case 119: { // w
      selectedVoice->dacValues[MOD_AMT] = constrain(selectedVoice->dacValues[MOD_AMT] + 25,
                                                    MOD_AMT_MIN_VALUE,
                                                    MOD_AMT_MAX_VALUE);
    }
    break;
    case 115: { // s
      selectedVoice->dacValues[MOD_AMT] = constrain(selectedVoice->dacValues[MOD_AMT] - 25,
                                                    MOD_AMT_MIN_VALUE,
                                                    MOD_AMT_MAX_VALUE);
    }
    break;
    // WAVE
    // When switching between SQR and other wave shapes, the PWM value needs to be swapped as well
    case 101: { // e
      if(selectedVoice->vco_shape == VCO_SHAPE_SQR){
        selectedVoice->square_pwm = selectedVoice->dacValues[PWM];
      } else {
        selectedVoice->other_pwm = selectedVoice->dacValues[PWM];
      }

      selectedVoice->vco_shape = constrain(selectedVoice->vco_shape + 1,
                                           VCO_SHAPE_SQR,
                                           VCO_SHAPE_SAW);
      selectedVoice->dacValues[WAVE_SELECT] = vco_shapes[selectedVoice->vco_shape];

      if(selectedVoice->vco_shape == VCO_SHAPE_SQR){
         selectedVoice->dacValues[PWM] = selectedVoice->square_pwm;
      } else {
        selectedVoice->dacValues[PWM] = selectedVoice->other_pwm;
      }
    }
    break;
    case 100: { // d
      if(selectedVoice->vco_shape == VCO_SHAPE_SQR){
        selectedVoice->square_pwm = selectedVoice->dacValues[PWM];
      } else {
        selectedVoice->other_pwm = selectedVoice->dacValues[PWM];
      }

      selectedVoice->vco_shape = constrain(selectedVoice->vco_shape - 1,
                                           VCO_SHAPE_SQR,
                                           VCO_SHAPE_SAW);
      selectedVoice->dacValues[WAVE_SELECT] = vco_shapes[selectedVoice->vco_shape];

      if(selectedVoice->vco_shape == VCO_SHAPE_SQR){
         selectedVoice->dacValues[PWM] = selectedVoice->square_pwm;
      } else {
        selectedVoice->dacValues[PWM] = selectedVoice->other_pwm;
      }
    }
    break;
    // PWM
    case 114: { // r
      selectedVoice->dacValues[PWM] = constrain(selectedVoice->dacValues[PWM] + 25,
                                                PWM_MIN_VALUE,
                                                PWM_MAX_VALUE);
    }
    break;
    case 102: { // f
      selectedVoice->dacValues[PWM] = constrain(selectedVoice->dacValues[PWM] - 25,
                                                PWM_MIN_VALUE,
                                                PWM_MAX_VALUE);
    }
    break;
    // MIXER
    case 116: { // t
      selectedVoice->dacValues[MIXER] = constrain(selectedVoice->dacValues[MIXER] + 25,
                                                  MIXER_MIN_VALUE,
                                                  MIXER_MAX_VALUE);
    }
    break;
    case 103: { // g
      selectedVoice->dacValues[MIXER] = constrain(selectedVoice->dacValues[MIXER] - 25,
                                                  MIXER_MIN_VALUE,
                                                  MIXER_MAX_VALUE);
    }
    break;
    // CUTOFF
    case 121: { // y
      selectedVoice->dacValues[CUTOFF] = constrain(selectedVoice->dacValues[CUTOFF] + 25,
                                                   CUTOFF_MIN_VALUE,
                                                   CUTOFF_MAX_VALUE);
    }
    break;
    case 104: { // h
      selectedVoice->dacValues[CUTOFF] = constrain(selectedVoice->dacValues[CUTOFF] - 25,
                                                   CUTOFF_MIN_VALUE,
                                                   CUTOFF_MAX_VALUE);
    }
    break;
    // RESONANCE
    case 117: { // u
      selectedVoice->dacValues[RESONANCE] = constrain(selectedVoice->dacValues[RESONANCE] + 25,
                                                      RESONANCE_MIN_VALUE,
                                                      RESONANCE_MAX_VALUE);
    }
    break;
    case 106: { // j
      selectedVoice->dacValues[RESONANCE] = constrain(selectedVoice->dacValues[RESONANCE] - 25,
                                                      RESONANCE_MIN_VALUE,
                                                      RESONANCE_MAX_VALUE);
    }
    break;
    // VCA
    case 105: { // i
      selectedVoice->dacValues[VCA] = constrain(selectedVoice->dacValues[VCA] + 25,
                                                VCA_MIN_VALUE,
                                                VCA_MAX_VALUE);
    }
    break;
    case 107: { // k
      selectedVoice->dacValues[VCA] = constrain(selectedVoice->dacValues[VCA] - 25,
                                                VCA_MIN_VALUE,
                                                VCA_MAX_VALUE);
    }
    break;
    case 111: { //o
      // Serial.print("selectedVoice->dac_offset:");
      // Serial.println(selectedVoice->dac_offset);
      selectedVoice->dac_offset = selectedVoice->dac_offset + AUTOTUNE_STEP_SIZE;
    }
    break;
    case 108: { //l
      selectedVoice->dac_offset = selectedVoice->dac_offset - AUTOTUNE_STEP_SIZE;
    }
    break;
    case 120: {} // x
    break;
    case 98:  {} // c
    break;
    case 118: {} // v
    break;
    case 99:  {} // b
    break;
    case 110: {} // n
    break;
    case 109: {} // m
    break;
    // case 65: { // A
    //   preMainLoop = 1; // stop envelopes
    //   setupAutotune(); // enable analogue comparator
    //   runAutotune();
    //   teardownAutotune();
    //   preMainLoop = 0; // start envelopes
    // }
    // break;
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
    case 79: { // O
      controlValues[CC_PWM]++;
    }
    break;
    case 76: { // L
      controlValues[CC_PWM]--;
    }
    break;


    case 122: { // z
        panic(); // zero DAC
      }
    break;
  }

  Serial.print("1/2/3");
  Serial.print("  CV: q/a");
  Serial.print("  MOD: w/s");
  Serial.print("  WAV: e/d");
  Serial.print("  PWM: r/f");
  Serial.print("  MIX: t/g");
  Serial.print("  VCF: y/h");
  Serial.print("  RES: u/j");
  Serial.print("  VCA: i/k");
  Serial.println("  OFS: o/l");
}
#endif
