#ifndef SETTING_H
#define SETTING_H

#ifndef Arduino_h
#include <Arduino.h>
#endif

#if DEBUG_LV == 1
#define debug_1(...) Serial.print(__VA_ARGS__)
#define debugln_1(...) Serial.println(__VA_ARGS__)
#define debug_2(...) Serial.print(__VA_ARGS__)
#define debugln_2(...) Serial.println(__VA_ARGS__)
#elif DEBUG_LV == 2
#define debug_1(...)
#define debugln_1(...)
#define debug_2(...) Serial.print(__VA_ARGS__)
#define debugln_2(...) Serial.println(__VA_ARGS__)
#else
#define debug_1(...)
#define debugln_1(...)
#define debug_2(...)
#define debugln_2(...)
#endif

/**
 * @brief CUSTOM_FUSE 를 설정합니다.
 * 커스텀 퓨즈 세팅을 사용할경우 1, 2
 * 사전 설정된 퓨즈세팅을 사용할 경우 0
 * #define CUSTOM_FUSE
 */
#if CUSTOM_FUSE == 1
#define ATmega32U4  // 커스텀 퓨즈세팅을 사용할 IC 를 정의합니다.
#include "fuse.h"
byte custom_fuses[5] = {
    low_fuses,
    high_fuses,
    extended_fuses,
    lock_bits,
};
#elif CUSTOM_FUSE == 2
String AVR_CORE = "";
byte custom_fuses_sd[5];
#endif

// copy of fuses/lock bytes found for this processor
byte fuses[5];

// Crossroads 보드를 사용하는 경우 LED 표시와 로터리 인코더가 있는지 여부를 지정합니다.
#define CROSSROADS_PROGRAMMING_BOARD false
// 파일 이름을 변경하는 데 로터리 인코더가 없는 보드인 경우 true로 설정합니다.
#define NO_ENCODER true
// 16진수 파일 이름을 사용하려는 경우 true로 설정합니다. (즉, 00에서 FF까지, 00에서 99가 아닌)
#define HEX_FILE_NAMES true

const bool allowTargetToRun = true;  // true이면 프로그래밍이 아닐 때 프로그래밍 라인이 해제됩니다.

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

const unsigned int ENTER_PROGRAMMING_ATTEMPTS = 2;

/* Port Setting Start ************************************/
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

// 타겟 IC에 프로그래밍을 시작하는 데 필요한 스위치
const byte startSwitch = 2;

// 세 가지 동작 상태 LED
const int errorLED = A0;    // RED
const int readyLED = A1;    // GREEN
const int workingLED = A2;  // BLUE

const int noLED = -1;  // 음수가 될 수 있음으로 int 여야 합니다!

// 프로그래밍 속도를 제어 값을 너무 줄이면 프로그래밍에 실패할 가능성이있습니다
const byte BB_DELAY_MICROSECONDS = 6;

// 타겟 보드 리셋에 사용한 핀
const byte RESET = MSPIM_SS;

// SD 칩 선택 핀
const uint8_t chipSelect = SS;
/* Port Setting End ************************************/

const unsigned long NO_PAGE = 0xFFFFFFFF;
const int MAX_FILENAME = 13;

// actions to take
enum {
    checkFile,
    verifyFlash,
    writeToFlash,
};
// meaning of positions in above array
enum { lowFuse, highFuse, extFuse, lockByte, calibrationByte };

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

// FreeRam() 함수는 사용 가능한 메모리 양을 반환
int FreeRam() {
    extern int __heap_start, *__brkval;
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
void printMemoryUsage() {
    debug_1("Free RAM: ");
    debug_1(FreeRam());
    debugln_1(" bytes");
}

#endif