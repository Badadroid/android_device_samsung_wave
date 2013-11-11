#include <fcntl.h>
#include <string.h>
#include <cutils/properties.h>
#include <cutils/log.h>

#define LOG_TAG "bdaddr"
#define RIL_BDADDR_PATH "/data/radio/bt.txt"
#define BDADDR_PATH "/data/bdaddr"

/* Read bluetooth MAC from RIL_BDADDR_PATH ,
 * write it to BDADDR_PATH, and set ro.bt.bdaddr_path to BDADDR_PATH
 *
 * Adapted from bdaddr_read.c of thunderg
 */

int main() {
    char bdaddr[18];
    int count;
    int fd;

    //waiting while RIL will create file with MAC adress
    usleep (1000*1000*5);

    fd = open(RIL_BDADDR_PATH, O_RDONLY);
    if(fd < 0) {
        fprintf(stderr, "open(%s) failed\n", RIL_BDADDR_PATH);
        ALOGE("Can't open %s\n", RIL_BDADDR_PATH);
        return -1;
    }

    count = read(fd, bdaddr, sizeof(bdaddr));
    if (count < 0) {
        fprintf(stderr, "read(%s) failed\n", RIL_BDADDR_PATH);
        ALOGE("Can't read %s\n", RIL_BDADDR_PATH);
        return -1;
    }
    else if (count != sizeof(bdaddr)) {
        fprintf(stderr, "read(%s) unexpected size %d\n", RIL_BDADDR_PATH, count);
        ALOGE("Error reading %s (unexpected size %d)\n", RIL_BDADDR_PATH, count);
        return -1;
    }

    fd = open(BDADDR_PATH, O_WRONLY|O_CREAT|O_TRUNC, 00600|00060|00006);
    if (fd < 0) {
        fprintf(stderr, "open(%s) failed\n", BDADDR_PATH);
        ALOGE("Can't open %s\n", BDADDR_PATH);
        return -2;
    }
    write(fd, bdaddr, 18);

    // Set bluetooth owner and permission
    fchown(fd, 1002, 1002);
    fchmod(fd, 0660);

    close(fd);
    property_set("ro.bt.bdaddr_path", BDADDR_PATH);
    return 0;
}
