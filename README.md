# pcc-kit
park's C compiler kit. compile C source code into arm64 / x86_64 binary, wrapper it into windows PE(.exe), linux elf, darwin macho(macOS / iOS).

### latest version: 0.3.001
### latest update: 2024-11-25
### project desc:
```
This project is for entertainment purposes and is not yet ready for production use. Initially, I wanted to name the project "micro c compiler kit," but that name was already taken, so it is now called "park's c compiler kit".
In terms of code structure, the compiler is divided into frontend and backend. The frontend is responsible for constructing the AST tree, generating intermediate code, and implementing optimizations. The backend aims to support the ARM64 and x86_64 instruction sets, though currently, only ARM64 is supported. The kit also includes an assembler and linker, implementing cross-platform support on different operating systems, with a target to support Linux, macOS, and Windows.
```
```
项目为娱乐目的，目前不能用于生产环境。一开始我想讲这个项目成为micro c compiler kit，但这个名字已经被捷足先登了。所以现在叫park's c compiler kit。
在代码结构上，编译器前后端分离，前端负责构造ast树、生成中间代码并实现优化。后端目标支持arm64与x86_64指令集，目前仅支持arm64；kit中还包含汇编器和链接器，实现在不同操作系统上的封装，目标支持linux、macos、windows
```
### supported host（编译平台）：
```
almost all, this project do not use any os platform or cpu feature.
```
### supported target（目标平台）:
```
windows 11 arm64
ubuntu 22 (linux 6.9) arm64
android (linux 6.1) arm64
macOS arm64 (not work due to code signature)
```

### compiler's syntax（语法）:
```
go to "docs" dir
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
pcc -S -O0 -o demo_add.s /path_to_your_source/demo_add.c
```
