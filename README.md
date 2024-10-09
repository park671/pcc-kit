# pcc-kit
park's C compiler kit.

This project is for entertainment purposes and is not yet ready for production use. Initially, I wanted to name the project "micro c compiler kit," but that name was already taken, so it is now called "park's c compiler kit".

In terms of code structure, the compiler is divided into frontend and backend. The frontend is responsible for constructing the AST tree, generating intermediate code, and implementing optimizations. The backend aims to support the ARM64 and x86_64 instruction sets, though currently, only ARM64 is supported. The kit also includes an assembler and linker, implementing cross-platform support on different operating systems, with a target to support Linux, macOS, and Windows.

项目为娱乐目的，目前不能用于生产环境。一开始我想讲这个项目成为micro c compiler kit，但这个名字已经被捷足先登了。所以现在叫park's c compiler kit。

在代码结构上，编译器前后端分离，前端负责构造ast树、生成中间代码并实现优化。后端目标支持arm64与x86_64指令集，目前仅支持arm64；kit中还包含汇编器和链接器，实现在不同操作系统上的封装，目标支持linux、macos、windows


编译器LL1

compiler's syntax:
```
## 程序和方法序列

<程序> → <方法序列>
<方法序列> → <方法定义> <方法序列> | ε

## 方法定义
方法支持多种返回类型和参数列表。

<方法定义> → <类型> <标识符> ( <参数列表> ) <复合语句>
<参数列表> → <参数定义> <更多参数> | ε
<更多参数> → , <参数定义> <更多参数> | ε
<参数定义> → <类型> <标识符>
<类型> → int | char | void | float | double

## 复合语句和语句序列
复合语句支持多条语句。

<复合语句> → { <语句序列> }
<语句序列> → <语句> <语句序列> | ε

## 语句
支持多种类型的语句。

<语句> → <if语句> | <while语句> | <for语句> | <复合语句> | <定义语句> | <赋值语句> | <方法调用> | <return语句>
<定义语句> → <类型> <标识符> = <表达式> ;
<表达式语句> → <表达式> ;
<方法调用> → <标识符> ( <实参列表> ) ;
<实参列表> → <算术表达式> <更多实参> | ε
<更多实参> → , <算术表达式> <更多实参> | ε
<return语句>  → return <算术表达式>

## 控制结构

<if语句> → if ( <布尔表达式> ) <语句> | if ( <布尔表达式> ) <语句> else <语句>
<while语句> → while ( <布尔表达式> ) <语句>
<for语句> → for ( <算术表达式> ; <布尔表达式> ; <算术表达式> ) <语句>

## 表达式和运算
表达式处理算术和布尔逻辑。

<布尔表达式> → <布尔项> <布尔表达式'>
<布尔表达式'> → || <布尔项> <布尔表达式'> | ε
<布尔项> → <布尔因子> <布尔项'>
<布尔项'> → && <布尔因子> <布尔项'> | ε
<布尔因子> → ! <布尔因子> | <算术表达式> <关系运算> <算术表达式>

<表达式> → <赋值表达式> | <算术表达式>

<赋值表达式> → <标识符> = <表达式>

<算术表达式> → <项> <算术表达式'>
<算术表达式'> → + <项> <算术表达式'> | - <项> <算术表达式'> | ε
<项> → <因子> <项'>
<项'> → * <因子> <项'> | / <因子> <项'> | % <因子> <项'> | ε
<因子> → <标识符> | <无符号整数> | ( <表达式> ) | <方法调用>

<关系运算> → < | > | <= | >= | == | !=

## 标识符和基础类型

<标识符> → <字母> <标识符'>
<标识符'> → <字母> <标识符'> | <数字> <标识符'> | ε
<无符号整数> → <数字> <无符号整数'> 
<无符号整数'> → <数字> <无符号整数'> | ε
<字母> → a | b | … | z | A | B | … | Z
<数字> → 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
```


here are some test code:

```
int add(int abc,     int cd33) {
    int c = abc + cd33;
    return c;
}

int main() {
    int index = 0;
    int test = 0;
    for(index = 0; index<10;index=index+1) {
        test = add(test, 1);
    }
    return test;
}
```

here are compiler's MIR result:
```angular2html
2024-10-09 21:02:29.199 - optimization:		---method:add---
2024-10-09 21:02:29.199 - mir:		type 6: _tv_0 = abc + cd33
2024-10-09 21:02:29.199 - mir:		ret: _tv_0
2024-10-09 21:02:29.199 - optimization:		---method:main---
2024-10-09 21:02:29.199 - mir:		type 4: index = 0
2024-10-09 21:02:29.199 - mir:		type 4: test = 0
2024-10-09 21:02:29.199 - mir:		type 4: index = 0
2024-10-09 21:02:29.199 - mir:			.opt flag:2
2024-10-09 21:02:29.199 - mir:		label:_lb_2
2024-10-09 21:02:29.199 - mir:		cmp: index < 10 ? _lb_0 : _lb_1
2024-10-09 21:02:29.199 - mir:		label:_lb_0
2024-10-09 21:02:29.199 - mir:		call: add(
2024-10-09 21:02:29.199 - mir:			test
2024-10-09 21:02:29.199 - mir:			1
2024-10-09 21:02:29.199 - mir:		)
2024-10-09 21:02:29.199 - mir:		type 6: test = [last ret]
2024-10-09 21:02:29.199 - mir:		type 4: _tv_16 = index + 1
2024-10-09 21:02:29.199 - mir:		type 4: index = _tv_16
2024-10-09 21:02:29.199 - mir:		jmp:_lb_2
2024-10-09 21:02:29.199 - mir:		label:_lb_1
2024-10-09 21:02:29.199 - mir:			.opt flag:3
2024-10-09 21:02:29.199 - mir:		ret: test
```

here are compiler's arm64 assembly code result:
```
.section .text
	.globl add
	.p2align 2
add:
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]
	add	x29, sp, #16
	str	w0, [sp, #12]
	str	w1, [sp, #8]
	ldr	w0, [sp, #12]
	ldr	w1, [sp, #8]
	add	w0, w0, w1
	str	w0, [sp, #4]
	mov	w0, w0
	ldp	x29, x30, [sp, #16]
	add	sp, sp, #32
	ret

	.globl main
	.p2align 2
main:
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]
	add	x29, sp, #16
	mov	w0, #0
	str	w0, [sp, #12]
	mov	w1, #0
	str	w1, [sp, #8]
	mov	w0, #0
	str	w0, [sp, #12]
;loop start
_lb_2:
	ldr	w0, [sp, #12]
	cmp	w0, #10
	b.lt	_lb_0
	b	_lb_1
_lb_0:
	ldr	w1, [sp, #8]
	mov	w0, w1
	mov	w1, #1
	bl	add
	mov	w0, w0
	str	w0, [sp, #8]
	ldr	w1, [sp, #12]
	add	w1, w1, #1
	str	w1, [sp, #4]
	mov	w2, w1
	str	w2, [sp, #12]
	b	_lb_2
_lb_1:
;loop end
	ldr	w0, [sp, #8]
	mov	w0, w0
	ldp	x29, x30, [sp, #16]
	add	sp, sp, #32
	ret
```

### how to build

this is a very simple cmake project, you should know how to build.
if u don't, then you should close this page and go to learn more, instead of read this code.

```
cmake build
```

### how to run
just read the "main"
here is a demo:
```
micro_cc -S -O0 -o demo_add.s /path_to_your_source/demo_add.c
```