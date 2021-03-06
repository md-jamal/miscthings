#include   <stdio.h>     
#include   <stdlib.h>   
#include   <unistd.h>     
#include   <sys/types.h> 
#include   <sys/stat.h>  
#include   <fcntl.h>    
#include   <termios.h>
#include   <errno.h>    
#include   <string.h>

void setTermios(struct termios * pNewtio, int uBaudRate)
{
    bzero(pNewtio, sizeof(struct termios)); /* clear struct for new port settings */
    //8N1
    pNewtio->c_cflag = uBaudRate | CS8 | CREAD | CLOCAL;
    pNewtio->c_iflag = IGNPAR;
    pNewtio->c_oflag = 0;
    pNewtio->c_lflag = 0; //non ICANON
    /*
     initialize all control characters
     default values can be found in /usr/include/termios.h, and
     are given in the comments, but we don't need them here
    */
    pNewtio->c_cc[VINTR] = 0; /* Ctrl-c */
    pNewtio->c_cc[VQUIT] = 0; /* Ctrl-\ */
    pNewtio->c_cc[VERASE] = 0; /* del */
    pNewtio->c_cc[VKILL] = 0; /* @ */
    pNewtio->c_cc[VEOF] = 4; /* Ctrl-d */
    pNewtio->c_cc[VTIME] = 5; /* inter-character timer, timeout VTIME*0.1 */
    pNewtio->c_cc[VMIN] = 0; /* blocking read until VMIN character arrives */
    pNewtio->c_cc[VSWTC] = 0; /* '\0' */
    pNewtio->c_cc[VSTART] = 0; /* Ctrl-q */
    pNewtio->c_cc[VSTOP] = 0; /* Ctrl-s */
    pNewtio->c_cc[VSUSP] = 0; /* Ctrl-z */
    pNewtio->c_cc[VEOL] = 0; /* '\0' */
    pNewtio->c_cc[VREPRINT] = 0; /* Ctrl-r */
    pNewtio->c_cc[VDISCARD] = 0; /* Ctrl-u */
    pNewtio->c_cc[VWERASE] = 0; /* Ctrl-w */
    pNewtio->c_cc[VLNEXT] = 0; /* Ctrl-v */
    pNewtio->c_cc[VEOL2] = 0; /* '\0' */
}

int main(int argc, char **argv)
{
    int fd;
    int nCount, nTotal, i;
    struct termios oldtio, newtio;
	
    char *dev ="/dev/pts/3";
	
    //if ((argc!=3) || (sscanf(argv[1], "%d", &nTotal) != 1)){
	//	printf("err: need tow arg!\n");
	//	return -1;
    //}

    if ((fd = open(dev, O_RDWR | O_NOCTTY))<0){		printf("err: can't open serial port!\n");
		return -1;
    }

    tcgetattr(fd, &oldtio); /* save current serial port settings */
	
    setTermios(&newtio, B38400);
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);

	//for (i=0; i<nTotal; i++){
	//	nCount=write(fd, argv[2], strlen(argv[2]));
	//	printf("send data\n");
	//	sleep(1);
	//}

    char cmd[7] = {0x0f, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
	
	printf ("Running...\n");

    while (1) {
	cmd[2] = 0x00;
	cmd[3] = 0x04;		// 0x04, left; 0x02, right; 0x08, up; 0x10, down
	cmd[4] = 0x2F;
	cmd[5] = 0x00;
	cmd[6] = ((cmd[1] + cmd[2] + cmd[3] + cmd[4] + cmd[5])) & 0xFF;	printf ("check sum: 0x%02x\n", cmd[6]);
	int ret = write (fd, cmd, 7);


	cmd[2] = 0x00;
	cmd[3] = 0x00;			//left and right stop
	cmd[4] = 0x2F;
	cmd[5] = 0x00;
	cmd[6] = ((cmd[1] + cmd[2] + cmd[3] + cmd[4] + cmd[5])) & 0xFF;	printf ("check sum: 0x%02x\n", cmd[6]);
	ret = write (fd, cmd, 7);

	cmd[2] = 0x00;
	cmd[3] = 0x51;			//get pos
	cmd[4] = 0x00;
	cmd[5] = 0x00;
	cmd[6] = ((cmd[1] + cmd[2] + cmd[3] + cmd[4] + cmd[5])) & 0xFF;	printf ("check sum: 0x%02x\n", cmd[6]);
	ret = write (fd, cmd, 7);

	usleep(1000);
	
	sync();
  }

	tcsetattr(fd, TCSANOW, &oldtio);
	close(fd);

    return 0;
}
