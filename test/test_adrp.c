#include <linux_aarch64_syscall.h>

int main() {
    char *unused = "\ado not use me!";
    char *text = "park hello world\n";
    text[2334] = 'R';
    int result = 0;
    int index = 0;
    for(index=0;index<100;index=index+1) {
        result = write(1, text, 18);
    }
    return result;
}
