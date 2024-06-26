#ifndef SIGNATURE_H
#define SIGNATURE_H

#ifndef Arduino_h
#include <Arduino.h>
#endif

typedef struct {
    byte sig[3];                  // 칩 시그니처
    char desc[14];                // 고정된 배열 크기로 칩 이름을 PROGMEM에 유지
    unsigned long flashSize;      // 플래시 크기 (바이트)
    unsigned int baseBootSize;    // 기본 부트로더 크기 (다른 부트로더는 2/4/8의 배수)
    unsigned long pageSize;       // 플래시 프로그래밍 페이지 크기 (바이트)
    byte fuseWithBootloaderSize;  // 예: lowFuse, highFuse, extFuse 중 하나
    bool timedWrites;             // 칩을 폴링하여 ready 상태가 되지 않는 경우 true
} signatureType;

const unsigned long kb = 1024;
const byte NO_FUSE = 0xFF;

// Atmega 칩들의 시그니처 코드입니다. 이 데이터들을 기준으로 타겟 칩을 인식합니다.
const signatureType signatures[] PROGMEM = {
    //     signature        description   flash size   bootloader  flash  fuse     timed
    //                                                     size    page    to      writes
    //                                                             size   change

    // Attiny84 family
    {{0x1E, 0x91, 0x0B}, "ATtiny24", 2 * kb, 0, 32, NO_FUSE, false},
    {{0x1E, 0x92, 0x07}, "ATtiny44", 4 * kb, 0, 64, NO_FUSE, false},
    {{0x1E, 0x93, 0x0C}, "ATtiny84", 8 * kb, 0, 64, NO_FUSE, false},

    // Attiny85 family
    {{0x1E, 0x91, 0x08}, "ATtiny25", 2 * kb, 0, 32, NO_FUSE, false},
    {{0x1E, 0x92, 0x06}, "ATtiny45", 4 * kb, 0, 64, NO_FUSE, false},
    {{0x1E, 0x93, 0x0B}, "ATtiny85", 8 * kb, 0, 64, NO_FUSE, false},

    // Atmega328 family
    {{0x1E, 0x92, 0x0A}, "ATmega48PA", 4 * kb, 0, 64, NO_FUSE, false},
    {{0x1E, 0x93, 0x0F}, "ATmega88PA", 8 * kb, 256, 128, extFuse, false},
    {{0x1E, 0x94, 0x0B}, "ATmega168PA", 16 * kb, 256, 128, extFuse, false},
    {{0x1E, 0x94, 0x06}, "ATmega168V", 16 * kb, 256, 128, extFuse, false},
    {{0x1E, 0x95, 0x0F}, "ATmega328P", 32 * kb, 512, 128, highFuse, false},
    {{0x1E, 0x95, 0x16}, "ATmega328PB", 32 * kb, 512, 128, highFuse, false},
    {{0x1E, 0x95, 0x14}, "ATmega328", 32 * kb, 512, 128, highFuse, false},

    // Atmega644 family
    {{0x1E, 0x94, 0x0A}, "ATmega164P", 16 * kb, 256, 128, highFuse, false},
    {{0x1E, 0x95, 0x08}, "ATmega324P", 32 * kb, 512, 128, highFuse, false},
    {{0x1E, 0x96, 0x0A}, "ATmega644P", 64 * kb, 1 * kb, 256, highFuse, false},

    // Atmega2560 family
    {{0x1E, 0x96, 0x08}, "ATmega640", 64 * kb, 1 * kb, 256, highFuse, false},
    {{0x1E, 0x97, 0x03}, "ATmega1280", 128 * kb, 1 * kb, 256, highFuse, false},
    {{0x1E, 0x97, 0x04}, "ATmega1281", 128 * kb, 1 * kb, 256, highFuse, false},
    {{0x1E, 0x98, 0x01}, "ATmega2560", 256 * kb, 1 * kb, 256, highFuse, false},

    {{0x1E, 0x98, 0x02}, "ATmega2561", 256 * kb, 1 * kb, 256, highFuse, false},

    // AT90USB family
    {{0x1E, 0x93, 0x82}, "At90USB82", 8 * kb, 512, 128, highFuse, false},
    {{0x1E, 0x94, 0x82}, "At90USB162", 16 * kb, 512, 128, highFuse, false},

    // Atmega32U2 family
    {{0x1E, 0x93, 0x89}, "ATmega8U2", 8 * kb, 512, 128, highFuse, false},
    {{0x1E, 0x94, 0x89}, "ATmega16U2", 16 * kb, 512, 128, highFuse, false},
    {{0x1E, 0x95, 0x8A}, "ATmega32U2", 32 * kb, 512, 128, highFuse, false},

    // Atmega32U4 family -  (datasheet is wrong about flash page size being 128 words)
    {{0x1E, 0x94, 0x88}, "ATmega16U4", 16 * kb, 512, 128, highFuse, false},
    {{0x1E, 0x95, 0x87}, "ATmega32U4", 32 * kb, 512, 128, highFuse, false},

    // ATmega1284P family
    {{0x1E, 0x97, 0x05}, "ATmega1284P", 128 * kb, 1 * kb, 256, highFuse, false},
    {{0x1E, 0x97, 0x06}, "ATmega1284", 128 * kb, 1 * kb, 256, highFuse, false},

    // ATtiny4313 family
    {{0x1E, 0x91, 0x0A}, "ATtiny2313A", 2 * kb, 0, 32, NO_FUSE, false},
    {{0x1E, 0x92, 0x0D}, "ATtiny4313", 4 * kb, 0, 64, NO_FUSE, false},

    // ATtiny13 family
    {{0x1E, 0x90, 0x07}, "ATtiny13A", 1 * kb, 0, 32, NO_FUSE, false},

    // Atmega8A family
    {{0x1E, 0x93, 0x07}, "ATmega8A", 8 * kb, 256, 64, highFuse, true},

    // ATmega64rfr2 family
    {{0x1E, 0xA6, 0x02}, "ATmega64rfr2", 256 * kb, 1 * kb, 256, highFuse, false},
    {{0x1E, 0xA7, 0x02}, "ATmega128rfr2", 256 * kb, 1 * kb, 256, highFuse, false},
    {{0x1E, 0xA8, 0x02}, "ATmega256rfr2", 256 * kb, 1 * kb, 256, highFuse, false},

};  // end of signatures

#endif