
#include <Arduino.h>
#include <SDFat.h>
#include <sdios.h>

#include "setting.h"

// SD 카드에서 읽을 고정 파일 이름 (루트 디렉터리)
const char wantedFile[] = "firmware.hex";
const char configFile[] = "config.ini";

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

// file system object
SdFat sd;

// 각 칩에 대한 시그니처 및 기타 관련 데이터를 저장하는 구조체
#include "signature.h"

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
        if (i < (repeat - 1)) delay(10);
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
            debugln_1("MSG_NO_SD_CARD");
            blink(errorLED, noLED, 1, 5);
            break;
        case MSG_CANNOT_OPEN_FILE:
            debugln_1("MSG_CANNOT_OPEN_FILE");
            blink(errorLED, noLED, 2, 5);
            while (!sd.begin(chipSelect, SPI_FULL_SPEED)) {
                ShowMessage(MSG_NO_SD_CARD);
                delay(500);
            }
            break;

        // problems reading the .hex file
        case MSG_LINE_TOO_LONG:
            debugln_1("MSG_LINE_TOO_LONG");
            blink(errorLED, workingLED, 1, 5);
            break;
        case MSG_LINE_TOO_SHORT:
            debugln_1("MSG_LINE_TOO_SHORT");
            blink(errorLED, workingLED, 2, 5);
            break;
        case MSG_LINE_DOES_NOT_START_WITH_COLON:
            debugln_1("MSG_LINE_DOES_NOT_START_WITH_COLON");
            blink(errorLED, workingLED, 3, 5);
            break;
        case MSG_INVALID_HEX_DIGITS:
            debugln_1("MSG_INVALID_HEX_DIGITS");
            blink(errorLED, workingLED, 4, 5);
            break;
        case MSG_BAD_SUMCHECK:
            debugln_1("MSG_BAD_SUMCHECK");
            blink(errorLED, workingLED, 5, 5);
            break;
        case MSG_LINE_NOT_EXPECTED_LENGTH:
            debugln_1("MSG_LINE_NOT_EXPECTED_LENGTH");
            blink(errorLED, workingLED, 6, 5);
            break;
        case MSG_UNKNOWN_RECORD_TYPE:
            debugln_1("MSG_UNKNOWN_RECORD_TYPE");
            blink(errorLED, workingLED, 7, 5);
            break;
        case MSG_NO_END_OF_FILE_RECORD:
            debugln_1("MSG_NO_END_OF_FILE_RECORD");
            blink(errorLED, workingLED, 8, 5);
            break;

        // problems with the file contents
        case MSG_FILE_TOO_LARGE_FOR_FLASH:
            debugln_1("MSG_FILE_TOO_LARGE_FOR_FLASH");
            blink(errorLED, workingLED, 9, 5);
            break;

        // problems programming the chip
        case MSG_CANNOT_ENTER_PROGRAMMING_MODE:
            debugln_1("MSG_CANNOT_ENTER_PROGRAMMING_MODE");
            blink(errorLED, noLED, 3, 5);
            break;
        case MSG_NO_BOOTLOADER_FUSE:
            debugln_1("MSG_NO_BOOTLOADER_FUSE");
            blink(errorLED, noLED, 4, 5);
            break;
        case MSG_CANNOT_FIND_SIGNATURE:
            debugln_1("MSG_CANNOT_FIND_SIGNATURE");
            blink(errorLED, noLED, 5, 5);
            break;
        case MSG_UNRECOGNIZED_SIGNATURE:
            debugln_1("MSG_UNRECOGNIZED_SIGNATURE");
            blink(errorLED, noLED, 6, 5);
            break;
        case MSG_BAD_START_ADDRESS:
            debugln_1("MSG_BAD_START_ADDRESS");
            blink(errorLED, noLED, 7, 5);
            break;
        case MSG_VERIFICATION_ERROR:
            debugln_1("MSG_VERIFICATION_ERROR");
            blink(errorLED, noLED, 8, 5);
            break;
        case MSG_FUSE_PROBLEM:
            debugln_1("MSG_FUSE_PROBLEM");
            blink(errorLED, noLED, 9, 5);
            break;
        case MSG_FLASHED_OK:
            debugln_1("MSG_FLASHED_OK");
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

#if CUSTOM_FUSE == 2
/*
ATmega328P:FFDEFDFF
*/
bool parseLine(const char *pLine) {
    char *token = strtok(const_cast<char *>(pLine), ":");
    if (token == nullptr) {
        // Error: No delimiter found
        return true;
    }
    debugln_2("\nDevice Setting from Config.ini File!!");
    String strToken(token);
    debug_2("IC : ");
    debugln_2(strToken);
    AVR_CORE = strToken;

    // Extract hex data
    token = strtok(nullptr, ":");
    if (token == nullptr) {
        // Error: No hex data found after delimiter
        return true;
    }
    String hexData(token);
    debug_2("Fuse Bits: ");
    debug_2(hexData);
    debugln_2(" (Low/High/Extended/Lock)");
    // Convert hex data to bytes
    char hexByte[3];
    int index = 0;
    while (hexData.length() >= 2) {
        hexByte[0] = hexData.charAt(0);
        hexByte[1] = hexData.charAt(1);
        hexByte[2] = '\0';
        byte value = strtol(hexByte, nullptr, 16);
        // debug_2("Byte: ");
        // debugln_2(value, HEX);
        custom_fuses_sd[index] = value;
        hexData.remove(0, 2);
        index++;
    }
    debugln_2("Device Setting End.\n");
    return false;
}

bool readConfigFile(const char *fName) {
    const int maxLine = 40;
    ifstream sdin(fName);
    char buffer[maxLine];

    if (!sdin.is_open()) {
        Serial.println("Config.ini not found!");
        return true;
    }

    while (sdin.getline(buffer, maxLine)) {
        int count = sdin.gcount();
        if (sdin.fail()) {
            debugln_2(MSG_LINE_TOO_LONG);
            return true;
        }
        if (count > 1) {
            if (parseLine(buffer)) {
                return true;
            }
        }
    }
    return false;
}
#endif

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
            debugln_1("MSG_VERIFY_FLASH");
            break;

        case writeToFlash:
            debugln_1("MSG_WRITE_TO_FLASH");
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
    debugln_1("MSG_START_PROGRAMMING");
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
            debugln_1("MSG_RECOGNIZED_SIGNATURE");
            program(loadExtendedAddressByte, 0, 0);
            return;
        }  // end of signature found
    }  // end of for each signature
    ShowMessage(MSG_UNRECOGNIZED_SIGNATURE);
}  // end of getSignature

void getFuseBytes() {
    debugln_2("Target Info Start!");
    String get_name = currentSignature.desc;
    debug_2("IC : ");
    debugln_2(get_name);
    debugln_2("current fuse bits set");
    fuses[lowFuse] = program(readLowFuseByte, readLowFuseByteArg2);
    fuses[highFuse] = program(readHighFuseByte, readHighFuseByteArg2);
    fuses[extFuse] = program(readExtendedFuseByte, readExtendedFuseByteArg2);
    fuses[lockByte] = program(readLockByte, readLockByteArg2);
    fuses[calibrationByte] = program(readCalibrationByte);
#if CUSTOM_FUSE == 1
    custom_fuses[calibrationByte] = program(readCalibrationByte);
#elif CUSTOM_FUSE == 2
    custom_fuses_sd[calibrationByte] = program(readCalibrationByte);
#endif
    debug_2(F("LFuse = 0x"));
    debugln_2(fuses[lowFuse], HEX);
    debug_2(F("HFuse = 0x"));
    debugln_2(fuses[highFuse], HEX);
    debug_2(F("EFuse = 0x"));
    debugln_2(fuses[extFuse], HEX);
    debug_2(F("Lock byte = "));
    debugln_2(fuses[lockByte], HEX);
    debug_2(F("Clock calibration = 0x"));
    debugln_2(fuses[calibrationByte], HEX);
    debugln_2("Target Info End");
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
        String get_name = currentSignature.desc;
#if CUSTOM_FUSE == 1  // 커스텀 퓨즈 비트설정
        if (get_name.indexOf(AVR_CORE, 0) != -1) {
            writeFuse(custom_fuses[lowFuse], fuseCommands[lowFuse]);
            writeFuse(custom_fuses[highFuse], fuseCommands[highFuse]);
            writeFuse(custom_fuses[extFuse], fuseCommands[extFuse]);
            writeFuse(custom_fuses[lockByte], fuseCommands[lockByte]);
        } else {
            debug_2("IC not Match");
            writeFuse(fuses[fusenumber], fuseCommands[fusenumber]);
        }
#elif CUSTOM_FUSE == 2
        if (get_name.indexOf(AVR_CORE, 0) != -1) {
            writeFuse(custom_fuses_sd[lowFuse], fuseCommands[lowFuse]);
            writeFuse(custom_fuses_sd[highFuse], fuseCommands[highFuse]);
            writeFuse(custom_fuses_sd[extFuse], fuseCommands[extFuse]);
            writeFuse(custom_fuses_sd[lockByte], fuseCommands[lockByte]);
        } else {
            debug_2("IC not Match");
            writeFuse(fuses[fusenumber], fuseCommands[fusenumber]);
        }
#else
        writeFuse(fuses[fusenumber], fuseCommands[fusenumber]);
#endif
    }

    return false;
}  // end of updateFuses

//------------------------------------------------------------------------------
//      SETUP
//------------------------------------------------------------------------------
void setup() {
#if SERIAL_DISABLE == false
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
    // if (readHexFile(name, verifyFlash)) return false;
    // now fix up fuses so we can boot
    if (errors == 0) updateFuses(true);

    return errors == 0;
}  // end of writeFlashContents

//------------------------------------------------------------------------------
//      LOOP
//------------------------------------------------------------------------------
void loop() {
    digitalWrite(readyLED, HIGH);
#if CUSTOM_FUSE == 2
    // config.ini 로 부터 퓨즈 데이터를 읽어옵니다.
    // 2KB 이하의 시스템 메모리의 IC 에서 DEBUG 옵션 true 사용시
    // FUSE_FROM_SD_CARD 기능을 활성화 하면 메모리 부족으로 장치가 리셋 될 수 있습니다.
    readConfigFile(configFile);
#endif
    // wait till they press the start switch
    while (digitalRead(startSwitch) == HIGH) {
    }  // end of waiting for switch press
    // printMemoryUsage(); // 메모리 출력
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