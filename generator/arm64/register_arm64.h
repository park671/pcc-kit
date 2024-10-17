//
// Created by Park Yu on 2024/10/11.
//

#ifndef PCC_REGISTER_ARM64_H
#define PCC_REGISTER_ARM64_H

// 64-bit X registers
extern int X0;
extern int X1;
extern int X2;
extern int X3;
extern int X4;
extern int X5;
extern int X6;
extern int X7;
extern int X8;
extern int X9;
extern int X10;
extern int X11;
extern int X12;
extern int X13;
extern int X14;
extern int X15;
extern int X16;
extern int X17;
extern int X18;
extern int X19;
extern int X20;
extern int X21;
extern int X22;
extern int X23;
extern int X24;
extern int X25;
extern int X26;
extern int X27;
extern int X28;
extern int X29;
extern int X30;
extern int XZR;
extern int SP;

#define COMMON_REG_SIZE 15

extern const int commonRegisterBinary[COMMON_REG_SIZE];

/**
 * assembly fields
 */
extern const char *commonRegisterName[COMMON_REG_SIZE];

extern const char *revertRegisterNames[];

//syscall num
extern const char *sysCallReg;

//jump method pointer, save the method pointer to this regs
//eg: BLR x16
extern const char *ipcRegs[];

//in some os, this reg is used for thread local data pointer
extern const char *platformReg;

//save current method sp
extern const char *framePointReg;

//in method call stack, this reg save the "caller method"'s loc,
//return instruction use x30 to jump back to the caller.
extern const char *linkReg;

#endif //PCC_REGISTER_ARM64_H
