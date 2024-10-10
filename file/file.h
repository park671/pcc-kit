//
// Created by Park Yu on 2024/10/10.
//

#ifndef MICRO_CC_FILE_H
#define MICRO_CC_FILE_H

extern void openFile(const char *fileName);

extern void writeFile(const char *format, ...);

extern void closeFile();

#endif //MICRO_CC_FILE_H
