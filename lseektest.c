#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2


int main() {
    int fd;
    char buf[20];

    // Open a file for writing
    fd = open("testfile.txt", O_CREATE | O_RDWR);
    if (fd < 0) {
        printf(1, "Error: Cannot open file\n");
        exit();
    }

    // Write data to file
    write(fd, "Hello, xv6!", 12);

    // Move file offset back by 5 bytes
    if (lseek(fd, -5, SEEK_CUR) < 0) {
        printf(1, "Error: lseek failed\n");
        close(fd);
        exit();
    }

    // Read from new position
    read(fd, buf, 5);
    buf[5] = '\0';  // Null-terminate string

    printf(1, "Read after lseek: %s\n", buf);  // Should print "xv6!"

    close(fd);
    exit();
}

