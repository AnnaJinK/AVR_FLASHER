## What is this?

<img src="PCB/img/1.png" width="60%"/>

SD 카드에 저장된 펌웨어를 타겟 AVR 장치로 플래싱하는 Standalone AVR Programmer 입니다.  
이 장치는 PC 가 필요없으며 간편하게 버튼 클릭만으로 타겟 장치의 IC 를 판별하여 펌웨어를 쓸수 있습니다.  
또한 ATtiny 부터 ATmega328PB 및 ATmega2560 등 ATmel 사의 다양한 AVR 을 지원합니다.

본 프로젝트는 [Gammon Forum](http://www.gammon.com.au/forum/?id=11638)의 내용을 바탕으로 제작되었습니다.

## Introduction

빌드된 펌웨어 파일 `firmaware.hex` 을 SD 카드에 옮겨준 후 장치에 삽입하고 버튼 만 누르면 됩니다.  
PlatformIO 에서 빌드된 펌웨어는 `firmaware.hex` 이름으로 `.pio/build` 폴더에 생성됩니다.  
ArduinoIDE 를 사용하는 경우 `Ctrl + Alt + S` 를 눌러 컴파일된 바이너리 파일 생성 후 `firmaware.hex` 로 이름을 바꿔서 사용하면 됩니다.

### Inside

PCB는 ATmega328P 제품을 사용하여 제작되었습니다.  
필요한 경우 ATmega2560 나 ATmega32u4 같은 다른 IC 를 사용하여 만들수도 있습니다.

<img src="PCB/img/3.png" width="40%"/>  
  
USB C 포트는 시리얼 통신을 지원하며, 아두이노 부트로더 내장시 USB 를 통해 펌웨어를 쉽게 수정할 수 있습니다.  
부트로더가 내장되지 않은 IC 를 사용하는 경우 PCB 에 실장되는 IC를 위한 ICSP 가 없기 때문에 사전에 프로그램을 다운받는 것이 좋습니다.  
저렴하고 구하기쉬운 최소한의 부품들로 구성하였기 때문에 누구나 쉽게만들 수 있습니다.

자세한 내용은 [회로도](PCB/Schematic.pdf)를 참조하세요.

회로도와 부품 배치 데이터, 케이스 3D 모델, 거버파일은 PCB 폴더에 있습니다.

### Video Link
[![Video](https://img.youtube.com/vi/CiSJQsz9dUg/0.jpg)](https://youtu.be/CiSJQsz9dUg)


### Available IC

```c
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

};
```
