#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    // Your test code here...
    int key = 1; // 示例key值
//    char data[100]; // 缓冲区用于写入和读取数据

    if(newsharedpage(key) < 0) {
        printf("newsharedpage failed\n");
    } else {
        printf("newsharedpage success\n");
    }

    // ... 测试其他系统调用 ...

    exit(0);
}
