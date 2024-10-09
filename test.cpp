//
// Created by Park Yu on 2024/9/27.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iostream>

#define N 110

using namespace std;

char code[N][N];
char reg[N];

int code_lines, reg_nums, reg_rest;

int is_in_reg(char var) {
    for (int i = 0; i < reg_rest; i++) {
        if (reg[i] == var)return i;
    }
    return -1;
}

int leastRecentlyUseLine(int now_line, char var) {
    for (int i = now_line; i < code_lines; i++) {
        if (code[i][3] == var || code[i][5] == var)return i;
    }
    return code_lines;
}

int ask_for_reg(int now_line) {
    if (reg_rest < reg_nums) {
        return reg_rest++;
    } else {
        int ans_pos = -1, ans_time = -1;
        int this_time;
        for (int i = 0; i < reg_nums; i++) {
            this_time = leastRecentlyUseLine(now_line, reg[i]);
            if (this_time > ans_time) {
                ans_time = this_time;
                ans_pos = i;
            }
        }
        return ans_pos;
    }
}

void show_op(char op) {
    switch (op) {
        case '+':
            cout << "ADD";
            break;
        case '-':
            cout << "SUB";
            break;
        case '*':
            cout << "MUL";
            break;
        case '\\':
            cout << "DIV";
            break;
        case '/':
            cout << "DIV";
            break;
    }
}

void show_right(char var) {
    int pos = is_in_reg(var);
    if (pos != -1) {
        printf("R%d\n", pos);
    } else {
        printf("%c\n", var);
    }
}

int main() {
    scanf("%d%d%*c", &code_lines, &reg_nums);
    reg_rest = 0;
    for (int i = 0; i < code_lines; i++) {
        scanf("%s", code[i]);
    }
    for (int i = 0; i < code_lines; i++) {
        int pos = is_in_reg(code[i][3]);
        if (pos == -1) {
            pos = ask_for_reg(i);
            if (reg[pos] && leastRecentlyUseLine(i, reg[pos]) < code_lines) {
                printf("ST R%d, %c\n", pos, reg[pos]);
            }
            printf("LD R%d, %c\n", pos, code[i][3]);
        }
        show_op(code[i][4]);
        printf(" R%d, ", pos);
        show_right(code[i][5]);
        reg[pos] = code[i][0];
    }
}