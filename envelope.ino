
void updateEnvelope(envelope_structure *envelope, voice *voice) {
  // Check gate
  switch(envelope->state) {
    case ENV_IDLE:
      break;
    case ENV_ATTACK:
      if(voice->gate) {

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
      if(voice->gate) {

        if((envelope->accumulator - (long)envelope->decay_rate) <= ((long)envelope->sustain_value << 4)) {
          envelope->state = ENV_SUSTAIN;
        } else {
          envelope->accumulator -= envelope->decay_rate;
          envelope->value = envelope->accumulator >> 4;
        }

      }
      break;
    case ENV_SUSTAIN:
      break;
    case ENV_RELEASE:
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
