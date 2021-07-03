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

uint16_t voice[6][8] = {
                        { 0, 0, 0, 0, 0, 0, 0, 0 },
                        { 0, 0, 0, 0, 0, 0, 0, 0 },
                        { 0, 0, 0, 0, 0, 0, 0, 0 },
                        { 0, 0, 0, 0, 0, 0, 0, 0 },
                        { 0, 0, 0, 0, 0, 0, 0, 0 },
                        { 0, 0, 0, 0, 0, 0, 0, 0 },
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

OutputPort<PORT_B> voiceParamSelect;                       // Arduino pin 8, 9, 10 to 4051 11, 10, 9
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
  OCR1A = 92;
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
uint8_t bufferBReadPointer = 0;

uint8_t voiceSelect = 0;
uint8_t voiceParamNumber = 0;

ISR(TIMER1_COMPA_vect) {                // interrupt commands for TIMER 1
  // DAC A

  voiceSelect = irq1Count >> 3;         // 0..5 => 0b000xxx ... 0b101xxx
  voiceParamNumber = irq1Count & 0x07;  // 0..7 => 0bxxx000 ... 0bxxx111

    m_outputChipSelectDAC = LOW;              // Disable DAC latch
    m_outputDisableMultiplex = HIGH;          // Disable Multiplex

  if(voiceSelect == 0 ) {
    uint16_t command = k_writeChannelA | (voice[0][voiceParamNumber] & 0x0FFF);

    voiceParamSelect.write(voiceParamNumber); // Select Channel
    // voiceParamSelect.write(irq1Count);        // Voice and Param Select
                                              // irq1Count is: xxxyyy (max 47 0b101111)
                                              // xxx is voice number
                                              // yyy is param number
    SPI.transfer(command >> 8);
    SPI.transfer(command & 0xFF);

    m_outputChipSelectDAC = HIGH;             // DAC latch Vout
    m_outputDisableMultiplex = LOW;           // Enable Multiplex

  }

  irq1Count++;
  if(irq1Count > 48)
    irq1Count = 0;

  bufferBReadPointer = 0;
  // DAC B
  while(data_available_B > 0) {
    uint16_t command = bufferB[bufferBReadPointer++];

    m_outputChipSelectDAC = LOW;              // Disable DAC latch

    SPI.transfer(command >> 8);
    SPI.transfer(command & 0xFF);

    m_outputChipSelectDAC = HIGH;             // DAC latch Vout
    data_available_B--;
  }
}

ISR(TIMER2_COMPA_vect){                 // interrupt commands for TIMER 2 here
  writeDACB(waveForm[LFO_SAW][lfoWavePointer]);

  lfoWavePointer++;
  if(lfoWavePointer == 120)
    lfoWavePointer = 0;
}

void writeDACB(uint16_t data){
  bufferB[data_available_B++] = k_writeChannelB | (data & 0x0FFF);
  if(data_available_B > 9) {
    data_available_B = 9;
  }
}

void setup() {

  m_outputChipSelectDAC = LOW;    // Disable DAC (~CS)
  m_outputDisableMultiplex = HIGH; // Disable Multiplex

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  Serial.begin(115200);
  Serial.println("Hello world");
  initializeInterrupts();
}

void updateVoice(uint16_t *voice) {
  //-4..+4V
  // voice[0][CV] = constrain((analogRead(A0) << 2) & 0xffc, 0, 4095);
  voice[CV] = 4095;

  //0..+4V
  // voice[MOD_AMT] = 2048;

  //-2..+4V
  // 0        => -2V
  // 1 (1024) => -1V Square
  // 2 (2048) =>  0V Triangle
  // 3 (4095) =>  1V Tri + Saw
  // 4 (????) =>  2V Saw
  // voice[WAVE_SELECT] = constrain(((analogRead(A0) & 0x3c0) << 2), 0, 4095);
  voice[WAVE_SELECT] = 4095;

  //0..+2V
  // voice[PWM] = 3072;
  // constrain(2048 + (waveForm[LFO_SINE][lfoWavePointer] >> 1), 2048, 4095);
  // voice[PWM] = constrain(2048 + (waveForm[LFO_SINE][lfoWavePointer] >> 1), 2048, 4095);

  //-2..+2V
  // voice[MIXER] = 0;

  //0..+2V
  // voice[RESONANCE] = 2048;

  //-3..+4V
  voice[CUTOFF] = constrain(4095 - waveForm[LFO_SINE][lfoWavePointer], 0, 4095);

  //0..+4V
  // voice[VCA] = constrain(2048 + (waveForm[LFO_TRI][lfoWavePointer] >> 1), 2048, 4095);
  voice[VCA] = 4095;
}

void loop() {
  updateVoice(voice[0]);
  delay(1);
  // //-4..+4V
  // // voice[0][CV] = constrain((analogRead(A0) << 2) & 0xffc, 0, 4095);
  // voice[0][CV] = 0;

  // //0..+4V
  // voice[0][MOD_AMT] = 2048;

  // //-2..+4V
  // // 0        => -2V
  // // 1 (1024) => -1V Square
  // // 2 (2048) =>  0V Triangle
  // // 3 (3072) =>  1V Tri + Saw
  // // 4 (4096) =>  2V Saw
  // voice[0][WAVE_SELECT] = constrain(((analogRead(A0) & 0x3c0) << 2), 0, 4095);

  // //0..+2V
  // voice[0][PWM] = 3072;
  // voice[0][PWM] = constrain(2048 + (waveForm[LFO_SINE][lfoWavePointer] >> 1), 2048, 4095);

  //-2..+2V
  // voice[0][MIXER] = 0;

  //0..+2V
  // voice[0][RESONANCE] = 2048;

  //-3..+4V
  // voice[0][CUTOFF] = constrain(4095 - ((analogRead(A1) & 0xffc) << 2), 0, 4095);

  //0..+4V
  // voice[0][VCA] = constrain(2048 + (waveForm[LFO_TRI][lfoWavePointer] >> 1), 2048, 4095);
  // voice[0][VCA] = 4095;
}

void writeCV(uint8_t channel, uint16_t param)
{
  uint16_t value_ = voice[param];
  m_outputChipSelectDAC = LOW;

  SPI.transfer(channel | (uint8_t)((value_ >> 8) & 0x0F));
  SPI.transfer((uint8_t)(value_ & 0xFF));
  m_outputDisableMultiplex = HIGH; // Disable Multiplex
  voiceParamSelect.write(param);      // Select Channel
  m_outputChipSelectDAC = HIGH;    // DAC latch Vout
  m_outputDisableMultiplex = LOW;  // Enable Multiplex
}
