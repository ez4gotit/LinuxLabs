#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DEVICE "/dev/int_stack"
#define IOCTL_MAGIC 0xF5
#define IOCTL_SET_SIZE _IOW(IOCTL_MAGIC, 0, int)

void usage() {
    fprintf(stderr, "Usage: kernel_stack [set-size|push|pop|unwind] <value>\n");
    exit(1);
}

int main(int argc, char **argv) {
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    if (argc < 2) usage();

    if (strcmp(argv[1], "set-size") == 0) {
        if (argc != 3) usage();
        int size = atoi(argv[2]);
        if (ioctl(fd, IOCTL_SET_SIZE, &size) < 0) {
            fprintf(stderr, "ERROR: size should be > 0\n");
            return 1;
        }
    } else if (strcmp(argv[1], "push") == 0) {
        if (argc != 3) usage();
        int val = atoi(argv[2]);
        if (write(fd, &val, sizeof(int)) < 0) {
            perror("ERROR");
            return -34; // -ERANGE
        }
    } else if (strcmp(argv[1], "pop") == 0) {
        int val;
        int ret = read(fd, &val, sizeof(int));
        if (ret == 0) {
            printf("NULL\n");
        } else {
            printf("%d\n", val);
        }
    } else if (strcmp(argv[1], "unwind") == 0) {
        int val;
        while (read(fd, &val, sizeof(int)) > 0)
            printf("%d\n", val);
    } else {
        usage();
    }

    close(fd);
    return 0;
}
