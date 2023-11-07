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



void test_free_shared_page(int key) {
    printf("开始测试 freesharedpage 系统调用\n");

    if(freesharedpage(key) < 0) {
        printf("第一次 freesharedpage 调用失败了，这不应该发生\n");
    } else {
        printf("第一次 freesharedpage 调用成功了，共享页面应该被释放\n");
    }
//    get_shared_page_info(key);
}

void test_get_shared_page(int key) {
    printf("开始测试 getsharedpageinfo 系统调用\n");
    get_shared_page_info(key);
}




int main(int argc, char *argv[]) {
    int key1 = 999; // 示例key值，用于创建新的共享页面
    int key2 = 1000; // 示例key值，用于创建新的共享页面

    test_new_shared_page(key1);
    test_new_shared_page(key2);
    test_free_shared_page(key1);
//    test_get_shared_page(key2);
//    test_get_shared_page(key1);

    exit(0);
}

