#include <linux_aarch64_syscall.h>

int add(int a, int b) {
    return a+b;
}

int main() {
    char *text = "park hello world\n";
    int result = 0;
    result = write(1, text, 18);
    return 11;
}
