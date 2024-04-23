
#include <Arduino.h>
#include <SDFat.h>
#include <sdios.h>

#define DEBUG false  // 디버그 모드

#if DEBUG == true
#define debug(...) Serial.print(__VA_ARGS__)
#define debugln(...) Serial.println(__VA_ARGS__)
#define debugBLE(...) sendBLE(__VA_ARGS__)
#else
#define debug(...)
#define debugln(...)
#define debugBLE(...)
#endif

// Crossroads 보드를 사용하는 경우 LED 표시와 로터리 인코더가 있는지 여부를 지정합니다.
#define CROSSROADS_PROGRAMMING_BOARD false
// 파일 이름을 변경하는 데 로터리 인코더가 없는 보드인 경우 true로 설정합니다.
#define NO_ENCODER true
// 16진수 파일 이름을 사용하려는 경우 true로 설정합니다. (즉, 00에서 FF까지, 00에서 99가 아닌)
#define HEX_FILE_NAMES true

const bool allowTargetToRun = true;  // true이면 프로그래밍이 아닐 때 프로그래밍 라인이 해제됩니다.

// SD 카드에서 읽을 고정 파일 이름 (루트 디렉터리)
const char wantedFile[] = "firmware.hex";

// 타겟 IC에 프로그래밍을 시작하는 데 필요한 스위치
const byte startSwitch = 2;

// 세 가지 동작 상태 LED
const int errorLED = A0;    // RED
const int readyLED = A1;    // GREEN
const int workingLED = A2;  // BLUE

const int noLED = -1;  // 음수가 될 수 있음으로 int 여야 합니다!

// status "messages"
typedef enum {
    MSG_NO_SD_CARD,                      // SD 카드를 열 수 없음
    MSG_CANNOT_OPEN_FILE,                // 'wantedFile' 파일을 열 수 없음 (위에 정의됨)
    MSG_LINE_TOO_LONG,                   // 디스크의 줄이 너무 깁니다.
    MSG_LINE_TOO_SHORT,                  // 유효하지 않은 줄이 너무 짧습니다.
    MSG_LINE_DOES_NOT_START_WITH_COLON,  // 줄이 콜론으로 시작하지 않습니다.
    MSG_INVALID_HEX_DIGITS,              // 16진수가 아닌 부분이 있습니다.
    MSG_BAD_SUMCHECK,                    // 줄의 체크섬이 잘못되었습니다.
    MSG_LINE_NOT_EXPECTED_LENGTH,        // 기대하는 길이의 레코드가 아닙니다.
    MSG_UNKNOWN_RECORD_TYPE,             // 알 수 없는 레코드 유형입니다.
    MSG_NO_END_OF_FILE_RECORD,           // 파일 끝에 'end of file'이 없습니다.
    MSG_FILE_TOO_LARGE_FOR_FLASH,        // 파일 크기가 플래시에 맞지 않습니다

    MSG_CANNOT_ENTER_PROGRAMMING_MODE,  // 타겟 칩을 프로그래밍할 수 없습니다.
    MSG_NO_BOOTLOADER_FUSE,             // 칩에 부트로더가 없습니다.
    MSG_CANNOT_FIND_SIGNATURE,          // 칩 서명을 찾을 수 없습니다.
    MSG_UNRECOGNIZED_SIGNATURE,         // 알 수 없는 서명입니다.
    MSG_BAD_START_ADDRESS,              // 파일 시작 주소가 잘못되었습니다.
    MSG_VERIFICATION_ERROR,             // 프로그래밍 후 확인 오류
    MSG_FUSE_PROBLEM,                   // 퓨즈 입력이 잘못되었거나 퓨즈 프로그래밍 문제
    MSG_FLASHED_OK,                     // 프로그래밍이 성공적으로 완료되었습니다.
} msgType;

/*
  For more details, photos, wiring, instructions, see:
    http://www.gammon.com.au/forum/?id=11638
  Copyright 2012 Nick Gammon. Addeded code forr fuse burning by Dan Geiger 2023.


  PERMISSION TO DISTRIBUTE

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
  and associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.


  LIMITATION OF LIABILITY

  The software is provided "as is", without warranty of any kind, express or implied,
  including but not limited to the warranties of merchantability, fitness for a particular
  purpose and noninfringement. In no event shall the authors or copyright holders be liable
  for any claim, damages or other liability, whether in an action of contract,
  tort or otherwise, arising from, out of or in connection with the software
  or the use or other dealings in the software.
*/

// const char Version[] = "1.3dg";

const unsigned int ENTER_PROGRAMMING_ATTEMPTS = 2;

// bit banged SPI pins
#ifdef __AVR_ATmega2560__
// Atmega2560
const byte MSPIM_SCK = 4;  // port G bit 5
const byte MSPIM_SS = 5;   // port E bit 3
const byte BB_MISO = 6;    // port H bit 3
const byte BB_MOSI = 7;    // port H bit 4
#elif defined(__AVR_ATmega1284P__)
// Atmega1284P
const byte MSPIM_SCK = 11;  // port D bit 3
const byte MSPIM_SS = 12;   // port D bit 4
const byte BB_MISO = 13;    // port D bit 5
const byte BB_MOSI = 14;    // port D bit 6
#else
// Atmega328
const byte MSPIM_SCK = 5;  // port D bit 5
const byte MSPIM_SS = 6;   // port D bit 6
const byte BB_MISO = 7;    // port D bit 7
const byte BB_MOSI = 8;    // port D bit 8
#endif

// 8 MHz clock on this pin
const byte CLOCKOUT = 9;

/*
대상 프로세서를 다음과 같이 연결하세요:
D5: (SCK) --> 데이터 시트에 따른 SCK
D6: (SS) --> 대상의 /RESET에 연결
D7: (MISO) --> 데이터 시트에 따른 MISO
D8: (MOSI) --> 데이터 시트에 따른 MOSI

D9: 대상에 필요한 경우 8 Mhz 클럭 신호

SD 카드를 다음과 같이 연결하세요:

D10: SS (칩 선택)
D11: MOSI (DI - SD 카드로 데이터 입력)
D12: MISO (DO - SD 카드에서 데이터 출력)
D13: SCK (CLK - 클럭)
SD 카드 및 대상 프로세서는 +5V 및 Gnd가 연결되어야 합니다.
D2와 Gnd 사이에 스위치를 연결하세요 (일반적으로 오픈 상태, 순간작동).
상태 LED:

A0: 빨간색 (오류) LED
A1: 녹색 (준비) LED
A2: 파란색 (작업 중) LED

각 LED에는 적절한 저항이 직렬로 연결되어야 합니다 (예: 220 옴).
LED의 다른 단자는 Gnd에 연결됩니다.
*/

// for fast port access
#ifdef __AVR_ATmega2560__
// Atmega2560
#define BB_MISO_PORT PINH
#define BB_MOSI_PORT PORTH
#define BB_SCK_PORT PORTG
const byte BB_SCK_BIT = 5;
const byte BB_MISO_BIT = 3;
const byte BB_MOSI_BIT = 4;
#elif defined(__AVR_ATmega1284P__)
// Atmega1284P
#define BB_MISO_PORT PIND
#define BB_MOSI_PORT PORTD
#define BB_SCK_PORT PORTD
const byte BB_SCK_BIT = 3;
const byte BB_MISO_BIT = 5;
const byte BB_MOSI_BIT = 6;
#else
// Atmega328
#define BB_MISO_PORT PIND
#define BB_MOSI_PORT PORTB
#define BB_SCK_PORT PORTD
const byte BB_SCK_BIT = 5;
const byte BB_MISO_BIT = 7;
const byte BB_MOSI_BIT = 0;
#endif

// 프로그래밍 속도를 제어 값을 너무 줄이면 프로그래밍에 실패할 가능성이있습니다
const byte BB_DELAY_MICROSECONDS = 4;

// 타겟 보드 리셋에 사용한 핀
const byte RESET = MSPIM_SS;

// SD 칩 선택 핀
const uint8_t chipSelect = SS;

const unsigned long NO_PAGE = 0xFFFFFFFF;
const int MAX_FILENAME = 13;

// actions to take
enum {
    checkFile,
    verifyFlash,
    writeToFlash,
};

// file system object
SdFat sd;

// copy of fuses/lock bytes found for this processor
byte fuses[5];

// meaning of positions in above array
enum { lowFuse, highFuse, extFuse, lockByte, calibrationByte };

// 각 칩에 대한 시그니처 및 기타 관련 데이터를 저장하는 구조체
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

char name[MAX_FILENAME] = {0};  // current file name

// number of items in an array
#define NUMITEMS(arg) ((unsigned int)(sizeof(arg) / sizeof(arg[0])))

//  SPI를 통해 타겟 칩으로 보낼 프로그래밍 명령어들입니다.
enum {
    progamEnable = 0xAC,

    // writes are preceded by progamEnable
    chipErase = 0x80,
    writeLockByte = 0xE0,
    writeLowFuseByte = 0xA0,
    writeHighFuseByte = 0xA8,
    writeExtendedFuseByte = 0xA4,

    pollReady = 0xF0,

    programAcknowledge = 0x53,

    readSignatureByte = 0x30,
    readCalibrationByte = 0x38,

    readLowFuseByte = 0x50,
    readLowFuseByteArg2 = 0x00,
    readExtendedFuseByte = 0x50,
    readExtendedFuseByteArg2 = 0x08,
    readHighFuseByte = 0x58,
    readHighFuseByteArg2 = 0x08,
    readLockByte = 0x58,
    readLockByteArg2 = 0x00,

    readProgramMemory = 0x20,
    writeProgramMemory = 0x4C,
    loadExtendedAddressByte = 0x4D,
    loadProgramMemory = 0x40,

};  // end of enum

// 어떤 프로그램 명령이 어떤 퓨즈를 쓰는지
const byte fuseCommands[4] = {writeLowFuseByte, writeHighFuseByte, writeExtendedFuseByte, writeLockByte};

// .hex 파일의 레코드 유형
enum {
    hexDataRecord,                    // 00
    hexEndOfFile,                     // 01
    hexExtendedSegmentAddressRecord,  // 02
    hexStartSegmentAddressRecord,     // 03
    hexExtendedLinearAddressRecord,   // 04
    hexStartLinearAddressRecord       // 05
};

// 함수 선언
void writeFuse(const byte newValue, const byte instruction);

//"times" 만큼 "interval" 지연시간으로 한 개 또는 두 개의 LED를 깜박입니다.
// 깜빡임 동작은 1초 대기 후 "repeat" 횟수만큼 다시 반복됩니다
void blink(const int whichLED1, const int whichLED2, const byte times = 1, const unsigned long repeat = 1, const unsigned long interval = 100) {
    for (unsigned long i = 0; i < repeat; i++) {
        for (byte j = 0; j < times; j++) {
            digitalWrite(whichLED1, HIGH);
            if (whichLED2 != noLED) digitalWrite(whichLED2, HIGH);
            delay(interval);
            digitalWrite(whichLED1, LOW);
            if (whichLED2 != noLED) digitalWrite(whichLED2, LOW);
            delay(interval);
        }  // end of for loop
        if (i < (repeat - 1)) delay(50);
    }  // end of repeating the sequence
}  // end of blink

void ShowMessage(const byte which) {
    // first turn off all LEDs
    digitalWrite(errorLED, LOW);
    digitalWrite(workingLED, LOW);
    digitalWrite(readyLED, LOW);

    // now flash an appropriate sequence
    switch (which) {
        // problems with SD card or finding the file
        case MSG_NO_SD_CARD:
            debugln("MSG_NO_SD_CARD");
            blink(errorLED, noLED, 1, 5);
            break;
        case MSG_CANNOT_OPEN_FILE:
            debugln("MSG_CANNOT_OPEN_FILE");
            blink(errorLED, noLED, 2, 5);
            while (!sd.begin(chipSelect, SPI_FULL_SPEED)) {
                ShowMessage(MSG_NO_SD_CARD);
                delay(500);
            }
            break;

        // problems reading the .hex file
        case MSG_LINE_TOO_LONG:
            debugln("MSG_LINE_TOO_LONG");
            blink(errorLED, workingLED, 1, 5);
            break;
        case MSG_LINE_TOO_SHORT:
            debugln("MSG_LINE_TOO_SHORT");
            blink(errorLED, workingLED, 2, 5);
            break;
        case MSG_LINE_DOES_NOT_START_WITH_COLON:
            debugln("MSG_LINE_DOES_NOT_START_WITH_COLON");
            blink(errorLED, workingLED, 3, 5);
            break;
        case MSG_INVALID_HEX_DIGITS:
            debugln("MSG_INVALID_HEX_DIGITS");
            blink(errorLED, workingLED, 4, 5);
            break;
        case MSG_BAD_SUMCHECK:
            debugln("MSG_BAD_SUMCHECK");
            blink(errorLED, workingLED, 5, 5);
            break;
        case MSG_LINE_NOT_EXPECTED_LENGTH:
            debugln("MSG_LINE_NOT_EXPECTED_LENGTH");
            blink(errorLED, workingLED, 6, 5);
            break;
        case MSG_UNKNOWN_RECORD_TYPE:
            debugln("MSG_UNKNOWN_RECORD_TYPE");
            blink(errorLED, workingLED, 7, 5);
            break;
        case MSG_NO_END_OF_FILE_RECORD:
            debugln("MSG_NO_END_OF_FILE_RECORD");
            blink(errorLED, workingLED, 8, 5);
            break;

        // problems with the file contents
        case MSG_FILE_TOO_LARGE_FOR_FLASH:
            debugln("MSG_FILE_TOO_LARGE_FOR_FLASH");
            blink(errorLED, workingLED, 9, 5);
            break;

        // problems programming the chip
        case MSG_CANNOT_ENTER_PROGRAMMING_MODE:
            debugln("MSG_CANNOT_ENTER_PROGRAMMING_MODE");
            blink(errorLED, noLED, 3, 5);
            break;
        case MSG_NO_BOOTLOADER_FUSE:
            debugln("MSG_NO_BOOTLOADER_FUSE");
            blink(errorLED, noLED, 4, 5);
            break;
        case MSG_CANNOT_FIND_SIGNATURE:
            debugln("MSG_CANNOT_FIND_SIGNATURE");
            blink(errorLED, noLED, 5, 5);
            break;
        case MSG_UNRECOGNIZED_SIGNATURE:
            debugln("MSG_UNRECOGNIZED_SIGNATURE");
            blink(errorLED, noLED, 6, 5);
            break;
        case MSG_BAD_START_ADDRESS:
            debugln("MSG_BAD_START_ADDRESS");
            blink(errorLED, noLED, 7, 5);
            break;
        case MSG_VERIFICATION_ERROR:
            debugln("MSG_VERIFICATION_ERROR");
            blink(errorLED, noLED, 8, 5);
            break;
        case MSG_FUSE_PROBLEM:
            debugln("MSG_FUSE_PROBLEM");
            blink(errorLED, noLED, 9, 5);
            break;
        case MSG_FLASHED_OK:
            debugln("MSG_FLASHED_OK");
            blink(readyLED, noLED, 3, 1);
            break;

        default:
            blink(errorLED, 11, 10);
            break;  // unknown error
    }  // end of switch on which message
}  // end of ShowMessage

// Bit Banged SPI transfer
byte BB_SPITransfer(byte c) {
    byte bit;

    for (bit = 0; bit < 8; bit++) {
        // write MOSI on falling edge of previous clock
        if (c & 0x80)
            BB_MOSI_PORT |= bit(BB_MOSI_BIT);
        else
            BB_MOSI_PORT &= ~bit(BB_MOSI_BIT);
        c <<= 1;

        // read MISO
        c |= (BB_MISO_PORT & bit(BB_MISO_BIT)) != 0;

        // clock high
        BB_SCK_PORT |= bit(BB_SCK_BIT);

        // delay between rise and fall of clock
        delayMicroseconds(BB_DELAY_MICROSECONDS);

        // clock low
        BB_SCK_PORT &= ~bit(BB_SCK_BIT);

        // delay between rise and fall of clock
        delayMicroseconds(BB_DELAY_MICROSECONDS);
    }

    return c;
}  // end of BB_SPITransfer

// 만약 위 테이블에서 시그니처를 찾았다면, foundSig 는 해당 시그니처의 인덱스입니다.
int foundSig = -1;
byte lastAddressMSB = 0;
// 일치하는 프로세서를 위한 현재 시그니처 항목의 사본입니다.
signatureType currentSignature;

// 한 개의 프로그래밍 명령어를 실행합니다. ... b1은 명령, b2, b3, b4는 인수입니다.
// 프로세서는 4번째 전송에서 결과를 반환할 수 있습니다. 이를 반환합니다.
byte program(const byte b1, const byte b2 = 0, const byte b3 = 0, const byte b4 = 0) {
    noInterrupts();
    BB_SPITransfer(b1);
    BB_SPITransfer(b2);
    BB_SPITransfer(b3);
    byte b = BB_SPITransfer(b4);
    interrupts();
    return b;
}  // end of program

// read a byte from flash memory
byte readFlash(unsigned long addr) {
    byte high = (addr & 1) ? 0x08 : 0;  // set if high byte wanted
    addr >>= 1;                         // turn into word address

    // set the extended (most significant) address byte if necessary
    byte MSB = (addr >> 16) & 0xFF;
    if (MSB != lastAddressMSB) {
        program(loadExtendedAddressByte, 0, MSB);
        lastAddressMSB = MSB;
    }  // end if different MSB

    return program(readProgramMemory | high, highByte(addr), lowByte(addr));
}  // end of readFlash

// write a byte to the flash memory buffer (ready for committing)
void writeFlash(unsigned long addr, const byte data) {
    byte high = (addr & 1) ? 0x08 : 0;  // set if high byte wanted
    addr >>= 1;                         // turn into word address
    program(loadProgramMemory | high, 0, lowByte(addr), data);
}  // end of writeFlash

// 두 개의 16진수 문자를 바이트로 변환합니다.
// 에러인 경우 true를 반환하고, 정상인 경우 false를 반환합니다.
bool hexConv(const char *(&pStr), byte &b) {
    if (!isxdigit(pStr[0]) || !isxdigit(pStr[1])) {
        ShowMessage(MSG_INVALID_HEX_DIGITS);
        return true;
    }  // end not hex

    b = *pStr++ - '0';
    if (b > 9) b -= 7;

    // high-order nybble
    b <<= 4;

    byte b1 = *pStr++ - '0';
    if (b1 > 9) b1 -= 7;

    b |= b1;

    return false;  // OK
}  // end of hexConv

// 타겟 장치가 프로그래밍할 준비가 될 때까지 대기합니다.
void pollUntilReady() {
    if (currentSignature.timedWrites)
        delay(10);  // at least 2 x WD_FLASH which is 4.5 mS
    else {
        while ((program(pollReady) & 1) == 1) {
        }  // wait till ready
    }  // end of if
}  // end of pollUntilReady

unsigned long pagesize;
unsigned long pagemask;
unsigned long oldPage;
unsigned int progressBarCount;

// shows progress, toggles working LED
void showProgress() { digitalWrite(workingLED, !digitalRead(workingLED)); }  // end of showProgress

// 모든 임시 페이지를 0xFF로 지웁니다. 만약 모두 기록하지 않는 경우를 대비합니다.
void clearPage() {
    unsigned int len = currentSignature.pageSize;
    for (unsigned int i = 0; i < len; i++) writeFlash(i, 0xFF);
}  // end of clearPage

// 페이지를 플래시 메모리에 기록합니다.
void commitPage(unsigned long addr) {
    addr >>= 1;  // turn into word address

    // set the extended (most significant) address byte if necessary
    byte MSB = (addr >> 16) & 0xFF;
    if (MSB != lastAddressMSB) {
        program(loadExtendedAddressByte, 0, MSB);
        lastAddressMSB = MSB;
    }  // end if different MSB

    showProgress();

    program(writeProgramMemory, highByte(addr), lowByte(addr));
    pollUntilReady();

    clearPage();  // clear ready for next page full
}  // end of commitPage

// 커밋할 준비가 된 임시 버퍼에 데이터를 씁니다.
void writeData(const unsigned long addr, const byte *pData, const int length) {
    // write each byte
    for (int i = 0; i < length; i++) {
        unsigned long thisPage = (addr + i) & pagemask;
        // page changed? commit old one
        if (thisPage != oldPage && oldPage != NO_PAGE) commitPage(oldPage);
        // now this is the current page
        oldPage = thisPage;
        // put byte into work buffer
        writeFlash(addr + i, pData[i]);
    }  // end of for

}  // end of writeData

// count errors
unsigned int errors;

void verifyData(const unsigned long addr, const byte *pData, const int length) {
    // check each byte
    for (int i = 0; i < length; i++) {
        unsigned long thisPage = (addr + i) & pagemask;
        // page changed? show progress
        if (thisPage != oldPage && oldPage != NO_PAGE) showProgress();
        // now this is the current page
        oldPage = thisPage;

        byte found = readFlash(addr + i);
        byte expected = pData[i];
        if (found != expected) {
            errors++;
        }
    }  // end of for

}  // end of verifyData

bool gotEndOfFile;
unsigned long extendedAddress;

unsigned long lowestAddress;
unsigned long highestAddress;
unsigned long bytesWritten;
unsigned int lineCount;

/*
  Line format:
  :nnaaaatt(data)ss

  Where:
  :      = a colon (hex line follows)
  F      = Fuse command follows (FaXX - a(L=low, H=high, E=extended, B=LockByte, C=calibration) - XX = value)

  (All of below in hex format)

  nn     = length of data part
  aaaa   = address (eg. where to write data)
  tt     = transaction type
           00 = data
           01 = end of file
           02 = extended segment address (changes high-order byte of the address)
           03 = start segment address *
           04 = linear address *
           05 = start linear address *
  (data) = variable length data
  ss     = sumcheck
              We don't use these

*/

// returns true if error, false if OK
bool processLine(const char *pLine, const byte action) {
    if (*pLine == 'F') {
        pLine++;
        // Get fuse type and value and program fuse

        byte fuseN = 20;
        byte fuseVal = 6;
        byte ATmega328PB_fuse[4] = {0xFF, 0xD6, 0xF5, 0xCF};

        switch (*pLine++) {
            case 'L':
                fuseN = lowFuse;
                break;
            case 'H':
                fuseN = highFuse;
                break;
            case 'E':
                fuseN = extFuse;
                break;
            case 'B':
                fuseN = lockByte;
                break;
            default:
                ShowMessage(MSG_FUSE_PROBLEM);  // Wrong Fuse code (L:Low; H=High; E=Extended; B=Lockbyte)
                return true;
        }

        switch (action) {
            case writeToFlash: {
                // Get fuse value and write
                if (!hexConv(pLine, fuseVal)) {
                    String name = currentSignature.desc;
                    if (name.indexOf("ATmega328PB", 0) != -1) {
                        debugln(currentSignature.desc);
                        writeFuse(ATmega328PB_fuse[fuseN], fuseCommands[fuseN]);
                    } else {
                        writeFuse(fuseVal, fuseCommands[fuseN]);
                    }
                    return false;
                } else {
                    ShowMessage(MSG_FUSE_PROBLEM);
                    return true;
                }
            }
            default:
                return false;
        }
    }

    if (*pLine++ != ':') {
        ShowMessage(MSG_LINE_DOES_NOT_START_WITH_COLON);
        return true;  // error
    }

    const int maxHexData = 40;
    byte hexBuffer[maxHexData];
    int bytesInLine = 0;

    if (action == checkFile)
        if (lineCount++ % 40 == 0) showProgress();

    // convert entire line from ASCII into binary
    while (isxdigit(*pLine)) {
        // can't fit?
        if (bytesInLine >= maxHexData) {
            ShowMessage(MSG_LINE_TOO_LONG);
            return true;
        }  // end if too long

        if (hexConv(pLine, hexBuffer[bytesInLine++])) return true;
    }  // end of while

    if (bytesInLine < 5) {
        ShowMessage(MSG_LINE_TOO_SHORT);
        return true;
    }

    // sumcheck it

    byte sumCheck = 0;
    for (int i = 0; i < (bytesInLine - 1); i++) sumCheck += hexBuffer[i];

    // 2's complement
    sumCheck = ~sumCheck + 1;

    // check sumcheck
    if (sumCheck != hexBuffer[bytesInLine - 1]) {
        ShowMessage(MSG_BAD_SUMCHECK);
        return true;
    }

    // length of data (eg. how much to write to memory)
    byte len = hexBuffer[0];

    // the data length should be the number of bytes, less
    //   length / address (2) / transaction type / sumcheck
    if (len != (bytesInLine - 5)) {
        ShowMessage(MSG_LINE_NOT_EXPECTED_LENGTH);
        return true;
    }

    // two bytes of address
    unsigned long addrH = hexBuffer[1];
    unsigned long addrL = hexBuffer[2];

    unsigned long addr = addrL | (addrH << 8);

    byte recType = hexBuffer[3];

    switch (recType) {
        // stuff to be written to memory
        case hexDataRecord:
            lowestAddress = min(lowestAddress, addr + extendedAddress);
            highestAddress = max(lowestAddress, addr + extendedAddress + len - 1);
            bytesWritten += len;

            switch (action) {
                case checkFile:  // nothing much to do, we do the checks anyway
                    break;

                case verifyFlash:
                    verifyData(addr + extendedAddress, &hexBuffer[4], len);
                    break;

                case writeToFlash:
                    writeData(addr + extendedAddress, &hexBuffer[4], len);
                    break;
            }  // end of switch on action
            break;

        // end of data
        case hexEndOfFile:
            gotEndOfFile = true;
            break;

        // we are setting the high-order byte of the address
        case hexExtendedSegmentAddressRecord:
            extendedAddress = ((unsigned long)hexBuffer[4]) << 12;
            break;

        // ignore these, who cares?
        case hexStartSegmentAddressRecord:
        case hexExtendedLinearAddressRecord:
        case hexStartLinearAddressRecord:
            break;

        default:
            ShowMessage(MSG_UNKNOWN_RECORD_TYPE);
            return true;
    }  // end of switch on recType

    return false;
}  // end of processLine

//------------------------------------------------------------------------------
// returns true if error, false if OK
bool readHexFile(const char *fName, const byte action) {
    const int maxLine = 80;
    char buffer[maxLine];
    ifstream sdin(fName);
    int lineNumber = 0;
    gotEndOfFile = false;
    extendedAddress = 0;
    errors = 0;
    lowestAddress = 0xFFFFFFFF;
    highestAddress = 0;
    bytesWritten = 0;
    progressBarCount = 0;

    pagesize = currentSignature.pageSize;
    pagemask = ~(pagesize - 1);
    oldPage = NO_PAGE;

    // check for open error
    if (!sdin.is_open()) {
        ShowMessage(MSG_CANNOT_OPEN_FILE);
        return true;
    }

    switch (action) {
        case checkFile:
            break;

        case verifyFlash:
            debugln("MSG_VERIFY_FLASH");
            break;

        case writeToFlash:
            debugln("MSG_WRITE_TO_FLASH");
            program(progamEnable, chipErase);  // erase it
            delay(20);                         // for Atmega8
            pollUntilReady();
            clearPage();  // clear temporary page
            break;
    }  // end of switch

    while (sdin.getline(buffer, maxLine)) {
        lineNumber++;

        int count = sdin.gcount();

        if (sdin.fail()) {
            ShowMessage(MSG_LINE_TOO_LONG);
            return true;
        }  // end of fail (line too long?)

        // ignore empty lines
        if (count > 1) {
            if (processLine(buffer, action)) {
                return true;  // error
            }
        }
    }  // end of while each line

    if (!gotEndOfFile) {
        ShowMessage(MSG_NO_END_OF_FILE_RECORD);
        return true;
    }

    switch (action) {
        case writeToFlash:
            // commit final page
            if (oldPage != NO_PAGE) commitPage(oldPage);
            break;

        case verifyFlash:
            if (errors > 0) {
                ShowMessage(MSG_VERIFICATION_ERROR);
                return true;
            }  // end if
            break;

        case checkFile:
            break;
    }  // end of switch

    return false;
}  // end of readHexFile

// returns true if managed to enter programming mode
bool startProgramming() {
    debugln("MSG_START_PROGRAMMING");
    byte confirm;
    pinMode(RESET, OUTPUT);
    digitalWrite(MSPIM_SCK, LOW);
    pinMode(MSPIM_SCK, OUTPUT);
    pinMode(BB_MOSI, OUTPUT);
    unsigned int timeout = 0;

    // we are in sync if we get back programAcknowledge on the third byte
    do {
        // regrouping pause
        delay(100);

        // ensure SCK low
        noInterrupts();
        digitalWrite(MSPIM_SCK, LOW);
        // then pulse reset, see page 309 of datasheet
        digitalWrite(RESET, HIGH);
        delayMicroseconds(10);  // pulse for at least 2 clock cycles
        digitalWrite(RESET, LOW);
        interrupts();

        delay(25);  // wait at least 20 mS
        noInterrupts();
        BB_SPITransfer(progamEnable);
        BB_SPITransfer(programAcknowledge);
        confirm = BB_SPITransfer(0);
        BB_SPITransfer(0);
        interrupts();

        if (confirm != programAcknowledge) {
            if (timeout++ >= ENTER_PROGRAMMING_ATTEMPTS) return false;
        }  // end of not entered programming mode

    } while (confirm != programAcknowledge);
    return true;  // entered programming mode OK
}  // end of startProgramming

void stopProgramming() {
    // turn off pull-ups
    digitalWrite(RESET, LOW);
    digitalWrite(MSPIM_SCK, LOW);
    digitalWrite(BB_MOSI, LOW);
    digitalWrite(BB_MISO, LOW);

    // set everything back to inputs
    pinMode(RESET, INPUT);
    pinMode(MSPIM_SCK, INPUT);
    pinMode(BB_MOSI, INPUT);
    pinMode(BB_MISO, INPUT);

}  // end of stopProgramming

void getSignature() {
    foundSig = -1;
    lastAddressMSB = 0;

    byte sig[3];
    for (byte i = 0; i < 3; i++) {
        sig[i] = program(readSignatureByte, 0, i);
    }  // end for each signature byte

    for (unsigned int j = 0; j < NUMITEMS(signatures); j++) {
        memcpy_P(&currentSignature, &signatures[j], sizeof currentSignature);

        if (memcmp(sig, currentSignature.sig, sizeof sig) == 0) {
            foundSig = j;
            // make sure extended address is zero to match lastAddressMSB variable
            debugln("MSG_RECOGNIZED_SIGNATURE");
            program(loadExtendedAddressByte, 0, 0);
            return;
        }  // end of signature found
    }  // end of for each signature
    ShowMessage(MSG_UNRECOGNIZED_SIGNATURE);
}  // end of getSignature

void getFuseBytes() {
    debugln("MSG_GET_FUSE");
    fuses[lowFuse] = program(readLowFuseByte, readLowFuseByteArg2);
    fuses[highFuse] = program(readHighFuseByte, readHighFuseByteArg2);
    fuses[extFuse] = program(readExtendedFuseByte, readExtendedFuseByteArg2);
    fuses[lockByte] = program(readLockByte, readLockByteArg2);
    fuses[calibrationByte] = program(readCalibrationByte);

    debug(F("LFuse = 0x"));
    debugln(fuses[lowFuse], HEX);
    debug(F("HFuse = 0x"));
    debugln(fuses[highFuse], HEX);
    debug(F("EFuse = 0x"));
    debugln(fuses[extFuse], HEX);
    debug(F("Lock byte = "));
    debugln(fuses[lockByte], HEX);
    debug(F("Clock calibration = 0x"));
    debugln(fuses[calibrationByte], HEX);
}  // end of getFuseBytes

// write specified value to specified fuse/lock byte
void writeFuse(const byte newValue, const byte instruction) {
    if (newValue == 0) return;  // ignore

    program(progamEnable, instruction, 0, newValue);
    pollUntilReady();
}  // end of writeFuse

// returns true if error, false if OK
bool updateFuses(const bool writeIt) {
    unsigned long addr;
    unsigned int len;

    byte fusenumber = currentSignature.fuseWithBootloaderSize;

    // if no fuse, can't change it
    if (fusenumber == NO_FUSE) {
        //    ShowMessage (MSG_NO_BOOTLOADER_FUSE);   // maybe this doesn't matter?
        return false;  // ok return
    }

    addr = currentSignature.flashSize;
    len = currentSignature.baseBootSize;

    if (lowestAddress == 0) {
        // don't use bootloader
        fuses[fusenumber] |= 1;
    } else {
        byte newval = 0xFF;

        if (lowestAddress == (addr - len))
            newval = 3;
        else if (lowestAddress == (addr - len * 2))
            newval = 2;
        else if (lowestAddress == (addr - len * 4))
            newval = 1;
        else if (lowestAddress == (addr - len * 8))
            newval = 0;
        else {
            ShowMessage(MSG_BAD_START_ADDRESS);
            return true;
        }

        if (newval != 0xFF) {
            newval <<= 1;
            fuses[fusenumber] &= ~0x07;  // also program (clear) "boot into bootloader" bit
            fuses[fusenumber] |= newval;
        }  // if valid

    }  // if not address 0

    if (writeIt) {
        // writeFuse(fuses[fusenumber], fuseCommands[fusenumber]);
        String name = currentSignature.desc;
        if (name.indexOf("ATmega328PB", 0) != -1) {
            debugln(currentSignature.desc);
            writeFuse(0xFF, fuseCommands[0]);
            writeFuse(0xD6, fuseCommands[1]);
            writeFuse(0xF5, fuseCommands[2]);
            writeFuse(0xCF, fuseCommands[3]);
        }
    }

    return false;
}  // end of updateFuses

//------------------------------------------------------------------------------
//      SETUP
//------------------------------------------------------------------------------
void setup() {
#if DEBUG == true
    Serial.begin(115200);
#endif
    pinMode(startSwitch, INPUT);
    digitalWrite(startSwitch, HIGH);
    pinMode(errorLED, OUTPUT);
    pinMode(readyLED, OUTPUT);
    pinMode(workingLED, OUTPUT);

    // 버스 오류를 피하기 위해 SD 카드를 SPI_HALF_SPEED로 초기화합니다.
    // 더 나은 성능을 위해 SPI_FULL_SPEED를 사용하세요.
    while (!sd.begin(chipSelect, SPI_FULL_SPEED)) {
        ShowMessage(MSG_NO_SD_CARD);
        delay(500);
    }
}  // end of setup

// returns true if error, false if OK
bool chooseInputFile() {
    if (readHexFile(name, checkFile)) {
        return true;  // error, don't attempt to write
    }

    // check file would fit into device memory
    if (highestAddress > currentSignature.flashSize) {
        ShowMessage(MSG_FILE_TOO_LARGE_FOR_FLASH);
        return true;
    }

    // check start address makes sense
    if (updateFuses(false)) {
        return true;
    }

    return false;
}  // end of chooseInputFile

// returns true if OK, false on error
bool writeFlashContents() {
    errors = 0;

    if (chooseInputFile()) return false;

    // ensure back in programming mode
    if (!startProgramming()) return false;

    // now commit to flash
    if (readHexFile(name, writeToFlash)) return false;

    // turn ready LED on during verification
    digitalWrite(readyLED, HIGH);

    // verify 프로그래밍 시간을 단축하길 원하면 제거항수 있습니다.
    if (readHexFile(name, verifyFlash)) return false;
    // now fix up fuses so we can boot
    if (errors == 0) updateFuses(true);

    return errors == 0;
}  // end of writeFlashContents

//------------------------------------------------------------------------------
//      LOOP
//------------------------------------------------------------------------------
void loop() {
    digitalWrite(readyLED, HIGH);

    // wait till they press the start switch
    while (digitalRead(startSwitch) == HIGH) {
    }  // end of waiting for switch press

    strcpy(name, wantedFile);  // use fixed name

    digitalWrite(readyLED, LOW);

    if (!startProgramming()) {
        ShowMessage(MSG_CANNOT_ENTER_PROGRAMMING_MODE);
        return;
    }  // end of could not enter programming mode

    getSignature();

    getFuseBytes();

    // don't have signature? don't proceed
    if (foundSig == -1) {
        ShowMessage(MSG_CANNOT_FIND_SIGNATURE);
        return;
    }  // end of no signature

    digitalWrite(workingLED, HIGH);
    bool ok = writeFlashContents();
    digitalWrite(workingLED, LOW);
    digitalWrite(readyLED, LOW);
    stopProgramming();
    delay(100);

    if (ok) {
        ShowMessage(MSG_FLASHED_OK);
    }

}  // end of loop