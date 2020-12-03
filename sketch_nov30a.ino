#include <SPI.h>
#include <DirectIO.h>

static uint16_t waveForm[120] =   {
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
  };
int lfoWavePointer = 0;
#define oneHzSample 1000000/120

int voice[] = {
              2047,   // Key CV
              2500,   // Mod Amount
              2750,   // Wave Select
              3000,   // PWM
              3250,   // Mixer Balance
              3500,   // Resonance
              3750,   // Cutoff
              4095    // VCA
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

OutputPort<PORT_B> channelSelect;
Output<k_pinChipSelectDAC>  m_outputChipSelectDAC;
Output<k_pinDisableMultiplex> m_outputDisableMultiplex;

static uint8_t k_writeChannelA = 0b01110000;
static uint8_t k_writeChannelB = 0b11110000;

uint8_t bpmTimeTablePrescalerArray[] = {
  (0 << CS12) | (1 << CS11) | (1 << CS10), // 64
  (1 << CS22) | (0 << CS21) | (0 << CS20)  // 256
};

uint16_t bufferA[10];
uint8_t data_available_A = 0;

uint16_t bufferB[10];
uint8_t data_available_B = 0;

void initializeInterrupts() {
  // TIMER 1 for interrupt frequency 10Hz:
  TIMSK1 = 0;
  TCCR1A = 0;                                         // set entire TCCR1A register to 0
  TCCR1B = 0;                                         // same for TCCR1B
  TCNT1  = 0;                                         // initialize counter value to 0
                                                      // set compare match register for bpm/60 Hz increments
                                                      // = 12000000 / (256 * freq) - 1 (must be < 65536)
  OCR1A = 389;                                        // 10Hz?
  TCCR1B |= (1 << WGM12);                             // turn on CTC mode
  TCCR1B |= bpmTimeTablePrescalerArray[1];            // Set CS12, CS11 and CS10 bits for prescaler
  TIMSK1 |= (1 << OCIE1A);                            // enable timer compare interrupt

  // TIMER 2 for interrupt frequency 100Hz:
  TCCR2B = 0;                 // set entire TCCR2B register to 0
  TCCR2A |= (1 << WGM21);     // turn on CTC mode
                              // set compare match register for 120 Hz increments (must be < 65536)
  OCR2A = 467;                // = 12000000 / (256 * 100) - 1
  TCNT2  = 0;                 // initialize counter value to 0
  TCCR2A = 0;                 // set entire TCCR2A register to 0
  TCCR2B |= bpmTimeTablePrescalerArray[1];    // Set CS22, CS21 and CS20 bits for 256 prescaler
  TIMSK2 |= (1 << OCIE2A);    // enable timer compare interrupt
}

ISR(TIMER1_COMPA_vect) {                // interrupt commands for TIMER 1

  writeDACB(waveForm[lfoWavePointer]);

  voice[CV] = waveForm[lfoWavePointer];
  data_available_A++;

  lfoWavePointer++;
  if(lfoWavePointer == 120)
    lfoWavePointer = 0;

//  writeCV(k_writeChannelA, CV);
//  writeCV(k_writeChannelA, MOD_AMT);
//  writeCV(k_writeChannelA, WAVE_SELECT);
//  writeCV(k_writeChannelA, PWM);
//  writeCV(k_writeChannelA, MIXER);
//  writeCV(k_writeChannelA, RESONANCE);
//  writeCV(k_writeChannelA, CUTOFF);
//  writeCV(k_writeChannelA, VCA);
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
//    Serial.println("CV");
//    writeCV(k_writeChannelA, CV);
    // delay(100);
//
//    Serial.println("MOD");
//    writeCV(k_writeChannelA, MOD_AMT);
//    delay(10);
//
//    Serial.println("WAVE");
//    writeCV(k_writeChannelA, WAVE_SELECT);
//    delay(10);
//
//    Serial.println("PWM");
//    writeCV(k_writeChannelA, PWM);
//    delay(10);
//
//    Serial.println("MIXER");
//    writeCV(k_writeChannelA, MIXER);
//    delay(10);
//
//    Serial.println("RESO");
//    writeCV(k_writeChannelA, RESONANCE);
//    delay(10);
//
//    Serial.println("CUTOFF");
//    writeCV(k_writeChannelA, CUTOFF);
//    delay(10);
//
//    Serial.println("VCA");
//    writeCV(k_writeChannelA, VCA);
//    delay(10);
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
  if(refreshVoicePointer > 7) {
    refreshVoicePointer = 0;
  }
  uint16_t command = (k_writeChannelA << 8) | (voice[refreshVoicePointer] & 0x0FFF);
  m_outputChipSelectDAC = LOW;                      // Disable DAC latch

  SPI.transfer(command >> 8);
  SPI.transfer(command & 0xFF);

  m_outputDisableMultiplex = HIGH;      // Disable Multiplex
  channelSelect.write(refreshVoicePointer);    // Select Channel
  m_outputChipSelectDAC = HIGH;         // DAC latch Vout
  m_outputDisableMultiplex = LOW;       // Enable Multiplex
  refreshVoicePointer++;

  read_pointer = 0;

  while(data_available_B > 0) {
    uint16_t command = bufferB[read_pointer++];

    m_outputChipSelectDAC = LOW;                      // Disable DAC latch

    SPI.transfer(command >> 8);
    SPI.transfer(command & 0xFF);

    m_outputChipSelectDAC = HIGH;                     // DAC latch Vout
    data_available_B--;
  }
}

void writeDACA(uint16_t data){
  uint16_t command = (k_writeChannelA << 8) | (data & 0x0FFF);
  bufferA[data_available_A++] = command;
  if(data_available_A > 9) {
    data_available_A = 9;
  }
}

void writeDACB(uint16_t data){
  bufferB[data_available_B++] = (k_writeChannelB << 8) | (data & 0x0FFF);
  if(data_available_B > 9) {
    data_available_B = 9;
  }
}
