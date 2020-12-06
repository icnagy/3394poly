#include <SPI.h>
#include <DirectIO.h>

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

int voice[] = {
              0,   // Key CV
              0,   // Mod Amount
              0,   // Wave Select
              0,   // PWM
              0,   // Mixer Balance
              0,   // Resonance
              0,   // Cutoff
              0    // VCA
            };

#define CV          0
#define MOD_AMT     1
#define WAVE_SELECT 2
#define PWM         3
#define MIXER       4
#define RESONANCE   5
#define CUTOFF      6
#define VCA         7

static constexpr unsigned k_pinDisableMultiplex = 6;
static constexpr unsigned k_pinChipSelectDAC = 7;

Input<0> waveformSelectInput;
Input<1> cutoffInput;

OutputPort<PORT_B> channelSelect;
Output<k_pinChipSelectDAC>  m_outputChipSelectDAC;
Output<k_pinDisableMultiplex> m_outputDisableMultiplex;

static uint8_t k_writeChannelA = 0b01110000;
static uint8_t k_writeChannelB = 0b11110000;

uint8_t bpmTimeTablePrescalerArray[] = {
  (0 << CS12) | (1 << CS11) | (1 << CS10), // 64
  (1 << CS22) | (0 << CS21) | (0 << CS20)  // 256
};

uint8_t skipVoiceUpdate = 0;

uint16_t bufferB[10];
uint8_t data_available_B = 0;

void initializeInterrupts() {
  // TIMER 1 for interrupt frequency 10Hz:
  TIMSK1 = 0;                               // disable timer compare interrupt
  TCCR1A = 0;                               // set entire TCCR1A register to 0
  TCCR1B = 0;                               // same for TCCR1B
  TCNT1  = 0;                               // initialize counter value to 0
                                            // set compare match register for bpm/60 Hz increments
                                            // = 12000000 / (256 * freq) - 1 (must be < 65536)
  OCR1A = 389;                              // 10Hz?
  TCCR1B |= (1 << WGM12);                   // turn on CTC mode
  TCCR1B |= bpmTimeTablePrescalerArray[1];  // Set CS12, CS11 and CS10 bits for prescaler
  TIMSK1 |= (1 << OCIE1A);                  // enable timer compare interrupt

  // TIMER 2 for interrupt frequency 100Hz:
  TCCR2B = 0;                               // set entire TCCR2B register to 0
  TCCR2A |= (1 << WGM21);                   // turn on CTC mode
                                            // set compare match register for 120 Hz increments (must be < 65536)
                                            // = 12000000 / (256 * 100) - 1
  OCR2A = 329;                              // 100 Hz
  TCNT2  = 0;                               // initialize counter value to 0
  TCCR2A = 0;                               // set entire TCCR2A register to 0
  TCCR2B |= bpmTimeTablePrescalerArray[1];  // Set CS22, CS21 and CS20 bits for 256 prescaler
  TIMSK2 |= (1 << OCIE2A);                  // enable timer compare interrupt
}

ISR(TIMER1_COMPA_vect) {                // interrupt commands for TIMER 1

  writeDACB(waveForm[1][lfoWavePointer]);

  voice[CV] = 0;  //waveForm[0][lfoWavePointer]
  voice[MOD_AMT] = constrain(0, 0, 2048);
  voice[WAVE_SELECT] = constrain((analogRead(A0) & 0xfffc) << 2, 0, 2048);
  voice[PWM] = constrain(waveForm[LFO_SINE][lfoWavePointer], 0, 2048);
  voice[MIXER] = 2048;
  voice[RESONANCE] = 1024;
  voice[CUTOFF] = constrain((analogRead(A1) & 0xfffc) << 2, 0, 2048);
  voice[VCA] = constrain(waveForm[LFO_TRI][lfoWavePointer], 0, 2048);

  lfoWavePointer++;
  if(lfoWavePointer == 120)
    lfoWavePointer = 0;
}

void setup() {

  m_outputChipSelectDAC = HIGH;   // Disable DAC (~CS)
  m_outputDisableMultiplex = LOW; // Enable Multiplex

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  Serial.begin(115200);
  cli(); // stop interrupts
  initializeInterrupts();
  sei(); // allow interrupts
}

void loop() {
  while(true) {
  }
}

void writeCV(uint8_t channel, uint16_t param)
{
  uint16_t value_ = voice[param];
  m_outputChipSelectDAC = LOW;

  SPI.transfer(channel | (uint8_t)((value_ >> 8) & 0x0F));
  SPI.transfer((uint8_t)(value_ & 0xFF));
  m_outputDisableMultiplex = HIGH; // Disable Multiplex
  channelSelect.write(param);      // Select Channel
  m_outputChipSelectDAC = HIGH;    // DAC latch Vout
  m_outputDisableMultiplex = LOW;  // Enable Multiplex
}

int refreshVoicePointer = 0;

ISR(TIMER2_COMPA_vect){                 // interrupt commands for TIMER 2 here

  int read_pointer = 0;
  // DAC A
  if (skipVoiceUpdate > 6) {
    if(refreshVoicePointer > 7) {
      refreshVoicePointer = 0;
      skipVoiceUpdate = 0;
    }
    uint16_t command = (k_writeChannelA << 8) | (voice[refreshVoicePointer] & 0x0FFF);
    m_outputChipSelectDAC = LOW;              // Disable DAC latch

    SPI.transfer(command >> 8);
    SPI.transfer(command & 0xFF);

    m_outputDisableMultiplex = HIGH;          // Disable Multiplex
    channelSelect.write(refreshVoicePointer); // Select Channel
    m_outputChipSelectDAC = HIGH;             // DAC latch Vout
    m_outputDisableMultiplex = LOW;           // Enable Multiplex
    refreshVoicePointer++;
  }
  skipVoiceUpdate++;

  read_pointer = 0;
  // DAC B
  while(data_available_B > 0) {
    uint16_t command = bufferB[read_pointer++];

    m_outputChipSelectDAC = LOW;              // Disable DAC latch

    SPI.transfer(command >> 8);
    SPI.transfer(command & 0xFF);

    m_outputChipSelectDAC = HIGH;             // DAC latch Vout
    data_available_B--;
  }
}

void writeDACB(uint16_t data){
  bufferB[data_available_B++] = (k_writeChannelB << 8) | (data & 0x0FFF);
  if(data_available_B > 9) {
    data_available_B = 9;
  }
}
