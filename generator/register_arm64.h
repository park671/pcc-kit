//
// Created by Park Yu on 2024/10/11.
//

#ifndef PCC_REGISTER_ARM64_H
#define PCC_REGISTER_ARM64_H

// 64-bit X registers
int X0 = 0x00;
int X1 = 0x01;
int X2 = 0x02;
int X3 = 0x03;
int X4 = 0x04;
int X5 = 0x05;
int X6 = 0x06;
int X7 = 0x07;
int X8 = 0x08;
int X9 = 0x09;
int X10 = 0x0A;
int X11 = 0x0B;
int X12 = 0x0C;
int X13 = 0x0D;
int X14 = 0x0E;
int X15 = 0x0F;
int X16 = 0x10;
int X17 = 0x11;
int X18 = 0x12;
int X19 = 0x13;
int X20 = 0x14;
int X21 = 0x15;
int X22 = 0x16;
int X23 = 0x17;
int X24 = 0x18;
int X25 = 0x19;
int X26 = 0x1A;
int X27 = 0x1B;
int X28 = 0x1C;
int X29 = 0x1D;
int X30 = 0x1E;
int XZR = 0x1F;
int SP = 0x1F;

#define COMMON_REG_SIZE 15

const int commonRegisterBinary[COMMON_REG_SIZE] = {
        //0  - 7
        X0,
        X1,
        X2,
        X3,
        X4,
        X5,
        X6,
        X7,
        //9  - 15
        X9,
        X10,
        X11,
        X12,
        X13,
        X14,
        X15,
};

/**
 * assembly fields
 */
const char *commonRegisterName[COMMON_REG_SIZE] = {
        //0  - 7
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        //9  - 15
        "9",
        "10",
        "11",
        "12",
        "13",
        "14",
        "15",
};

//syscall num
const char *sysCallReg = "x8";

//jump method pointer, save the method pointer to this regs
//eg: BLR x16
const char *ipcRegs[] = {
        "x16",
        "x17"
};

//in some os, this reg is used for thread local data pointer
const char *platformReg = "x18";

//save current method sp
const char *framePointReg = "x29";

//in method call stack, this reg save the "caller method"'s loc,
//return instruction use x30 to jump back to the caller.
const char *linkReg = "x30";


#endif //PCC_REGISTER_ARM64_H
