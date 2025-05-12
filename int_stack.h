#ifndef INT_STACK_H
#define INT_STACK_H

#include <linux/ioctl.h>

#define DEVICE_PATH "/dev/int_stack"
#define IOCTL_MAGIC 0xF5
#define IOCTL_SET_SIZE _IOW(IOCTL_MAGIC, 0, int)

#endif // INT_STACK_H
