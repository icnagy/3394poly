#define TAB     "\t\t"
#define NEWTAB    "\n" TAB

int findVoiceWithNote(uint8_t note) {
  for(int i = 0; i < NUMBER_OF_VOICES; i++) {
    if(voicess[i].gate == NOTE_ON && voicess[i].note == note)
      return i;
  }
  return -1;
}

int findIdleVoice() {
  for(int i = 0; i < NUMBER_OF_VOICES; i++) {
    if(voicess[i].gate == NOTE_OFF && (voicess[i].vca_envelope.state == ENV_IDLE || voicess[i].vca_envelope.state == ENV_RELEASE))
      return i;
  }
  return -1;
}

// Pitch is 3/4V per octave or 0.750V per octave
// An octave has 12 semitones, each semitone is 0.750V / 12 = 0.0625V
// 0.0625V would be 62.5 steps on the DAC
// #define NOTE_TO_DAC_VALUE(note) CV_MAX_VALUE - (((125 * note) >> 1) - controlValues[CC_FINE])
#define NOTE_TO_DAC_VALUE(note) CV_MAX_VALUE - (((125 * note) >> 1))

void voiceNoteOn(int voiceNo, uint8_t note, uint8_t velocity) {
  voicess[voiceNo].gate = NOTE_ON;
  voicess[voiceNo].note = note;
  voicess[voiceNo].dacValues[CV] = constrain(NOTE_TO_DAC_VALUE(note),
                                             CV_MIN_VALUE,
                                             CV_MAX_VALUE);
  // Pluss add all pitch changing values (ie: coarse, fine tune)
#ifdef DISPLAY_MIDI_DATA
  Serial.println(voicess[voiceNo].dacValues[CV]);
#endif
  voicess[voiceNo].velocity = velocity;
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

/*****************************************************************************/
/** Note On                                                                 **/
/*****************************************************************************/
void miby_note_on( miby_this_t midi_state )
{
  // printf( "<Ch.%d> ", MIBY_CHAN(midi_state));
  // printf( "Note On :: note = %02X, vel = %02X\n", MIBY_ARG0(midi_state),
  //                                                 MIBY_ARG1(midi_state));
  if(MIBY_ARG1(midi_state) == 0) {
    miby_note_off(midi_state);
  }
  else {
    uint8_t voiceNumber = 0xFF;
    voiceNumber = findIdleVoice();

#ifdef DISPLAY_MIDI_DATA
    Serial.print( "<Ch.");
    Serial.print(MIBY_CHAN(midi_state));
    Serial.print( "> Note On :: note = ");
    Serial.print(MIBY_ARG0(midi_state));
    Serial.print(", vel = ");
    Serial.print(MIBY_ARG1(midi_state));
    Serial.print(", voice = ");
    Serial.println(voiceNumber);
#endif
    if(voiceNumber == 0xFF)
      return;
    else {
      voiceNoteOn(voiceNumber,
                  MIBY_ARG0(midi_state),
                  MIBY_ARG1(midi_state));
#ifdef DISPLAY_VOICE_DATA
      printVoiceDacValues(&voicess[0]);
      printVoiceDacValues(&voicess[1]);
      printVoiceDacValues(&voicess[2]);
      printVoiceDacValues(&voicess[3]);
      printVoiceDacValues(&voicess[4]);
      printVoiceDacValues(&voicess[5]);
      Serial.println("------------------------------");
#endif
    }
  }
}

/*****************************************************************************/
/** Note Off                                                                **/
/*****************************************************************************/
void miby_note_off( miby_this_t midi_state )
{
  // printf( "<Ch.%d> ", MIBY_CHAN(midi_state) );
  // printf( "Note Off :: note = %02X, vel = %02X\n", MIBY_ARG0(midi_state),
  //                                                  MIBY_ARG1(midi_state) );
  uint8_t voiceNumber = findVoiceWithNote(MIBY_ARG0(midi_state));

#ifdef DISPLAY_MIDI_DATA
  Serial.print( "<Ch.");
  Serial.print(MIBY_CHAN(midi_state));
  Serial.print( "> Note Off :: note = ");
  Serial.print(MIBY_ARG0(midi_state));
  Serial.print(", vel = ");
  Serial.print(MIBY_ARG1(midi_state));
  Serial.print(", voice = ");
  Serial.println(voiceNumber);
#endif

  if(voiceNumber == 0xFF)
    return;
  else {
    voiceNoteOff(voiceNumber,
                MIBY_ARG0(midi_state),
                MIBY_ARG1(midi_state));
#ifdef DISPLAY_VOICE_DATA
    printVoiceDacValues(&voicess[0]);
    printVoiceDacValues(&voicess[1]);
    printVoiceDacValues(&voicess[2]);
    printVoiceDacValues(&voicess[3]);
    printVoiceDacValues(&voicess[4]);
    printVoiceDacValues(&voicess[5]);
    Serial.println("------------------------------");
#endif
  }

}

/*****************************************************************************/
/** Realtime System Reset                                                   **/
/*****************************************************************************/
void miby_rt_system_reset( miby_this_t sss )
{
  Serial.println("[FF] SysRT: system reset" );
  panic();
}

/*****************************************************************************/
/** Realtime Timing Clock                                                   **/
/*****************************************************************************/
// void test_rt_timing_clock( miby_this_t sss )
// {
//   Serial.println("[F8] SysRT: timing clock" );
// }

/*****************************************************************************/
/** Realtime Start                                                          **/
/*****************************************************************************/
// void test_rt_start( miby_this_t sss )
// {
//   Serial.println("[FA] SysRT: start" );
// }

/*****************************************************************************/
/** Realtime Continue                                                       **/
/*****************************************************************************/
// void test_rt_continue( miby_this_t sss )
// {
//   Serial.println("[FB] SysRT: continue" );
// }

/*****************************************************************************/
/** Realtime Stop                                                           **/
/*****************************************************************************/
// void test_rt_stop( miby_this_t sss )
// {
//   Serial.println("[FC] SysRT: stop" );
// }

/*****************************************************************************/
/** Realtime Active Sense                                                   **/
/*****************************************************************************/
// void test_rt_active_sense( miby_this_t sss )
// {
//   Serial.println( NEWTAB "[FE] SysRT: active sense" );
// }

/*****************************************************************************/
/** MIDI Time Code                                                          **/
/*****************************************************************************/
// void test_mtc( miby_this_t sss )
// {
//   printf( NEWTAB "[%02X %02X] ", MIBY_STATUSBYTE(sss), MIBY_ARG0(sss) );
//   printf( "SysCom: Timecode :: type = %02X, value = %02X\n", MIBY_ARG0(sss) >> 4, MIBY_ARG0(sss) & 0xF );
// }

/*****************************************************************************/
/** Song Position                                                           **/
/*****************************************************************************/
// void test_songpos( miby_this_t sss )
// {
//   printf( NEWTAB "[%02X %02X %02X] ", MIBY_STATUSBYTE(sss), MIBY_ARG0(sss), MIBY_ARG1(sss) );
//   printf( "SysCom: Song Position :: LSB = %02X, MSB = %02X\n", MIBY_ARG0(sss), MIBY_ARG1(sss) );
// }

/*****************************************************************************/
/** Song Select                                                             **/
/*****************************************************************************/
// void test_songsel( miby_this_t sss )
// {
//   printf( NEWTAB "[%02X %02X] ", MIBY_STATUSBYTE(sss), MIBY_ARG0(sss) );
//   printf( "SysCom: Song Select :: song %02X\n", MIBY_ARG0(sss) );
// }

/*****************************************************************************/
/** Tune Request                                                            **/
/*****************************************************************************/
// void test_tunereq( miby_this_t sss )
// {
//   printf( NEWTAB "[%02X] ", MIBY_STATUSBYTE(sss) );
//   printf( "SysCom: Tune Request\n" );
// }

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
#ifdef DISPLAY_MIDI_DATA
  Serial.print(MIBY_CHAN(sss));
  Serial.print(MIBY_ARG0(sss));
  Serial.println(MIBY_ARG1(sss));
#endif
  if(MIBY_ARG0(sss) < 34) {
    controlValues[MIBY_ARG0(sss)] = MIBY_ARG1(sss);
  }
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
#ifdef DISPLAY_MIDI_DATA
  Serial.print("PB:");
  Serial.print(MIBY_CHAN(sss));
  Serial.print(MIBY_ARG1(sss));
  Serial.println(MIBY_ARG0(sss));
#endif
}

/*****************************************************************************/
/** System Exclusive                                                        **/
/*****************************************************************************/
void test_sysex( miby_this_t sss )
{
  int i;
  static char *sysex_state[] = { "IDLE", "START", "MID", "END", "ABORT" };

  // printf( NEWTAB "[F0] SYSEX chunk: state = %02X (%s), length = %02X bytes\n",
  //       MIBY_SYSEX_STATE(sss),
  //       sysex_state[MIBY_SYSEX_STATE(sss)],
  //       MIBY_SYSEX_LEN(sss) );
  // printf( TAB "\t[" );
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
