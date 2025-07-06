### Issues
- ch02. QEMU, gcc, nasm, binutil의 최신 버전을 사용해도 아직은(~ ch24) 문제가 없다
```
Linux archlinux 6.15.3-arch1-1 #1 SMP PREEMPT_DYNAMIC Thu, 19 Jun 2025 14:41:19 +0000 x86_64 GNU/Linux

qemu-x86_64 version 10.0.0
Copyright (c) 2003-2025 Fabrice Bellard and the QEMU Project developers

gcc (GCC) 15.1.1 20250425
Copyright (C) 2025 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

GNU ld (GNU Binutils) 2.44.0
Copyright (C) 2025 Free Software Foundation, Inc.
This program is free software; you may redistribute it under the terms of
the GNU General Public License version 3 or (at your option) a later version.
This program has absolutely no warranty.

NASM version 2.16.03 compiled on May 13 2025
```
- ch05. 플로피 디스크 섹터 수가 더이상 18이 아니다. 36이다
    - BootLoader.asm 19 -> 37
- ch08. QEMU에서는 A20 Gate 활성화가 필요하지 않음
- ch11. Keyboard LED 구현 안함
- ch11. Keyboard A20 구현 안함
- ch11. QEMU -graphic curses에서는 SHIFT를 눌러도 scan code가 가지 않음
    - SHIFT를 누른 체로 다른 키도 눌러야 그 때 연속해서 scan code가 인식됨
- ch12. 391p kInitializeTSSSegment에 오타
    - TSSDATA -> TSSSEGMENT
- ch13. 링킹 중 `InterruptHandler.c`에서 `__stack_chk_fail`에러 발생
    - `02.Kernel64/Makefile`의 GCC64에 `-fno-stack-protector` 추가
- ch24. `kWriteHDDSector`에서 `kWaitForHDDNoBusy`전에 `kLock`해야함
- ch25. `Main`에서 `FileSystem`를 초기화하면 `HDD`를 더이상 초기화 할 필요 없음
