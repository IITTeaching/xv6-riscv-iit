#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void test_new_shared_page(int key) {
    printf("开始测试 newsharedpage 系统调用\n");

    if(newsharedpage(key) < 0) {
        printf("第一次 newsharedpage 调用失败了，这不应该发生\n");
    } else {
        printf("第一次 newsharedpage 调用成功了，共享页面应该被创建\n");
    }

    if(newsharedpage(key) < 0) {
        printf("第二次 newsharedpage 调用失败了，这是正确的，因为共享页面已存在\n");
    } else {
        printf("第二次 newsharedpage 调用成功了，这不应该发生，因为共享页面已存在\n");
    }

    get_shared_page_info(key);
}


int main(int argc, char *argv[]) {
    int key = 999; // 示例key值，用于创建新的共享页面

    test_new_shared_page(key);

    exit(0);
}
