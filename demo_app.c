#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define BUF_SIZE 128

int main(int argc, char *argv[])
{
    int fd;
    char read_buf[BUF_SIZE];

    /* 参数检查 */
    if (argc < 3) {
        printf("Usage: %s <device_path> <write_string>\n", argv[0]);
        printf("Example: %s /dev/hello_char \"hello kernel\"\n", argv[0]);
        return -1;
    }

    const char *dev_path = argv[1];
    const char *write_buf = argv[2];

    /* 1. open */
    fd = open(dev_path, O_RDWR);
    if (fd < 0) {
        perror("open failed");
        return -1;
    }

    printf("open %s success, fd = %d\n", dev_path, fd);

    /* 2. write */
    ssize_t ret = write(fd, write_buf, strlen(write_buf));
    if (ret < 0) {
        perror("write failed");
        close(fd);
        return -1;
    }

    printf("write %ld bytes: %s\n", ret, write_buf);

    /* 3. read */
    memset(read_buf, 0, BUF_SIZE);
    ret = read(fd, read_buf, BUF_SIZE);
    if (ret < 0) {
        perror("read failed");
        close(fd);
        return -1;
    }

    printf("read %ld bytes\n", ret);

    /* 4. close */
    close(fd);
    printf("device closed\n");

    return 0;
}
