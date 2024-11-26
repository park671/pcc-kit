#include <linux_aarch64_syscall.h>

int add(int a, int b) {
    return a + b;
}

int main() {
    char *read = "123456\n";
    int result = 0;
    result = read(0, read, 4);
    result = write(1, read, 7);
    result = add(result, 1);
    return result;
}
