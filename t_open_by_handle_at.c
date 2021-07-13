#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)

/* Scan /proc/self/mountinfo to find the line whose mount ID matches
   aqmount_idaq. (An easier way to do this is to install and use the
   aqlibmountaq library provided by the aqutil-linuxaq project.)
   Open the corresponding mount path and return the resulting file
   descriptor. */

static int
open_mount_path_by_id(int mount_id)
{
    char *linep;
    size_t lsize;
    char mount_path[PATH_MAX];
    int mi_mount_id, found;
    ssize_t nread;
    FILE *fp;

    fp = fopen("/proc/self/mountinfo", "r");
    if (fp == NULL)
        errExit("fopen");

    found = 0;
    linep = NULL;
    while (!found) {
        nread = getline(&linep, &lsize, fp);
        if (nread == -1)
            break;

        nread = sscanf(linep, "%d %*d %*s %*s %s",
                       &mi_mount_id, mount_path);
        if (nread != 2) {
            fprintf(stderr, "Bad sscanf()\n");
            exit(EXIT_FAILURE);
        }

        if (mi_mount_id == mount_id)
            found = 1;
    }
    free(linep);

    fclose(fp);

    if (!found) {
        fprintf(stderr, "Could not find mount point\n");
        exit(EXIT_FAILURE);
    }

    return open(mount_path, O_RDONLY);
}

int
main(int argc, char *argv[])
{
    struct file_handle *fhp;
    int mount_id, fd, mount_fd, handle_bytes, j;
    ssize_t nread;
    char buf[1000];
#define LINE_SIZE 100
    char line1[LINE_SIZE], line2[LINE_SIZE];
    char *nextp;

    if ((argc > 1 && strcmp(argv[1], "--help") == 0) || argc > 2) {
        fprintf(stderr, "Usage: %s [mount-path]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Standard input contains mount ID and file handle information:

         Line 1: <mount_id>
         Line 2: <handle_bytes> <handle_type>   <bytes of handle in hex>
    */

    if ((fgets(line1, sizeof(line1), stdin) == NULL) ||
           (fgets(line2, sizeof(line2), stdin) == NULL)) {
        fprintf(stderr, "Missing mount_id / file handle\n");
        exit(EXIT_FAILURE);
    }

    mount_id = atoi(line1);

    handle_bytes = strtoul(line2, &nextp, 0);

    /* Given handle_bytes, we can now allocate file_handle structure */

    fhp = malloc(sizeof(struct file_handle) + handle_bytes);
    if (fhp == NULL)
        errExit("malloc");

    fhp->handle_bytes = handle_bytes;

    fhp->handle_type = strtoul(nextp, &nextp, 0);

    for (j = 0; j < fhp->handle_bytes; j++)
        fhp->f_handle[j] = strtoul(nextp, &nextp, 16);

    /* Obtain file descriptor for mount point, either by opening
       the pathname specified on the command line, or by scanning
       /proc/self/mounts to find a mount that matches the aqmount_idaq
       that we received from stdin. */

    if (argc > 1)
        mount_fd = open(argv[1], O_RDONLY);
    else
        mount_fd = open_mount_path_by_id(mount_id);

    if (mount_fd == -1)
        errExit("opening mount fd");

    /* Open file using handle and mount point */

    fd = open_by_handle_at(mount_fd, fhp, O_RDONLY);
    if (fd == -1)
        errExit("open_by_handle_at");

    /* Try reading a few bytes from the file */

    nread = read(fd, buf, sizeof(buf));
    if (nread == -1)
        errExit("read");

    printf("Read %zd bytes\n", nread);

    exit(EXIT_SUCCESS);
}
