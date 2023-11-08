#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void test_new_shared_page(int key) {
    printf("Starting test for newsharedpage system call\n");

    if(newsharedpage(key) < 0) {
        printf("First call to newsharedpage failed, this should not happen\n");
    } else {
        printf("First call to newsharedpage succeeded, shared page should be created\n");
    }

    if(newsharedpage(key) < 0) {
        printf("Second call to newsharedpage failed, which is correct, because shared page already exists\n");
    } else {
        printf("Second call to newsharedpage succeeded, this should not happen as the shared page already exists\n");
    }

    get_shared_page_info(key);
}

void test_read_shared_page(int key) {
    printf("Starting test for readsharedpage system call\n");

    // Assumed constants - adjust as necessary.
    const int offset = 0;
    const int data_size = 100; // Size of the data to read.
    char buffer[data_size]; // Buffer to read into.

    // Read from the shared page.
    if (readsharedpage(key, offset, data_size, buffer) < 0) {
        printf("readsharedpage call failed\n");
    } else {
        printf("readsharedpage call succeeded\n");

        // If you have written data to the shared page previously, you can compare it here.
        // For example, if you wrote "Hello, World!" into the shared page, check like this:
        // if (strncmp(buffer, "Hello, World!", data_size) == 0) {
        //     printf("The read data matches expectations\n");
        // } else {
        //     printf("The read data does not match expectations\n");
        // }

        // For now, just print out the buffer for verification.
        printf("Data read: '%s'\n", buffer);
    }
}

void test_write_shared_page(int key, char *data) {
    printf("Starting test for writesharedpage system call\n");

    // Assumed constants - adjust as necessary.
    const int offset = 0;
    const int data_size = 100; // Size of the data to write.
    char buffer[data_size]; // Buffer containing data to write.

    // Initialize the buffer with data to write.
    // For the sake of an example, let's assume we want to write "Hello, World!"
    strcpy(buffer, data);

    // Write to the shared page.
    if (writesharedpage(key, offset, data_size, buffer) < 0) {
        printf("writesharedpage call failed\n");
    } else {
        printf("writesharedpage call succeeded\n");

        // If the system call includes verification of write, you can check here.
        // Since we don't read back in this test, we cannot verify the data here.
        // Verification would need a subsequent call to readsharedpage.

        // For now, we just assume the write was successful.
        printf("Data written: '%s'\n", buffer);
    }
}

void test_free_shared_page(int key) {
    printf("Starting test for freesharedpage system call\n");

    if(freesharedpage(key) < 0) {
        printf("First call to freesharedpage failed, this should not happen\n");
    } else {
        printf("First call to freesharedpage succeeded, shared page should be released\n");
    }
//    get_shared_page_info(key);
}

void test_get_shared_page(int key) {
    printf("Starting test for getsharedpageinfo system call\n");
    get_shared_page_info(key);
}

int main(int argc, char *argv[]) {
    int key1 = 999; // Example key value for creating a new shared page
    int key2 = 1000; // Another example key value for creating a new shared page

    test_new_shared_page(key1);
    test_new_shared_page(key2);
    test_write_shared_page(key1, "Hello, World!"); // Test writing to the shared page.
    test_write_shared_page(key2, "cs450 success"); // Test writing to the shared page.
    test_read_shared_page(key1); // Test reading from the shared page.
    test_read_shared_page(key2); // Test reading from the shared page.
    test_free_shared_page(key1);
    test_free_shared_page(key2);
//    test_get_shared_page(key2);
//    test_get_shared_page(key1);

    exit(0);
}
