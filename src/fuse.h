/* Add your fuse bits settings as follows: */
#ifdef ATmega328P_UNO
const char AVR_CORE[] = "ATmega328P";
const byte low_fuses = 0xFF;
const byte high_fuses = high_fuses 0xDE;
const byte extended_fuses = 0xFD;
const byte lock_bits = 0xFF;
#endif

#ifdef ATmega328P
const char AVR_CORE[] = "ATmega328P";
const byte low_fuses = 0xFF;
const byte high_fuses = high_fuses 0xDA;
const byte extended_fuses = 0xFD;
const byte lock_bits = 0xFF;
#endif

#ifdef ATmega328PB
const char AVR_CORE[] = "ATmega328PB";
const byte low_fuses = 0xFF;
const byte high_fuses = high_fuses 0xDE;
const byte extended_fuses = 0xFC;
const byte lock_bits = 0xFF;
#endif

#ifdef ATmega168P
const char AVR_CORE[] = "ATmega168P";
const byte low_fuses = 0xFF;
const byte high_fuses = high_fuses 0xDD;
const byte extended_fuses = 0xF8;
const byte lock_bits = 0xFF;
#endif

#ifdef ATmega32U4
const char AVR_CORE[] = "ATmega32U4";
const byte low_fuses = 0xFF;
const byte high_fuses = 0xD8;
const byte extended_fuses = 0xCB;
const byte lock_bits = 0xFF;
#endif
