//
// Created by Park Yu on 2024/10/15.
//
#include "register_arm64.h"

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

const char *revertRegisterNames[] = {
        "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
        "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
        "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
        "x24", "x25", "x26", "x27", "x28", "x29", "x30", "sp"  // xzr和sp同值，使用sp
};

const char *sysCallReg = "x8";
const char *ipcRegs[] = {"x16", "x17"};
const char *platformReg = "x18";
const char *framePointReg = "x29";
const char *linkReg = "x30";
