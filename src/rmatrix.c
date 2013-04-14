/* MC Li modified the C code from Mark Williams for running in R environment
   See http://github.com/mwilliams03/8x8matrixscroll

Use:
   sudo R

   dyn.load("rmatrix.so")
   out=.C("led8x8", as.character("Hello World"))
*/

#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "i2c-dev.h"
#include <fcntl.h>
#include "8x8font.h"
#include <string.h>
#include <time.h>

__u16 block[I2C_SMBUS_BLOCK_MAX];
//global variables used for matrix
int res, i2cbus, daddress, address, size, file;

//Reverse the bits
unsigned  char  reverseBits(unsigned  char num)
{
    unsigned  char count = sizeof(num) * 8 - 1;
    unsigned  char reverse_num = num;
    num >>= 1;
    while(num)
    {
       reverse_num <<= 1;
       reverse_num |= num & 1;
       num >>= 1;
       count--;
    }
    reverse_num <<= count;
    return reverse_num;
}

/* Print n as a binary number */
void printbitssimple(char n) {
        unsigned char i;
        i = 1<<(sizeof(n) * 8 - 1);

        while (i > 0) {
                if (n & i)
                        printf("#");
                else
                        printf(".");
                i >>= 1;
        	}
        printf("\n");
}


int displayImage(__u16 bmp[], int res, int daddress, int file)
{
        int i;
        for(i=0; i<8; i++)
        {
             block[i] = (bmp[i]&0xfe) >>1 |
             (bmp[i]&0x01) << 7;
        }
        res = i2c_smbus_write_i2c_block_data(file, daddress, 16,
                (__u8 *)block);
        usleep(100000);
}

void  INThandler(int sig)
{
       // Closing file and turning off Matrix

	unsigned short int clear[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

	displayImage(clear,res, daddress, file);

        printf("Closing file and turning off the LED Matrix\n");
        daddress = 0x20;
        for(daddress = 0xef; daddress >= 0xe0; daddress--) {
                res = i2c_smbus_write_byte(file, daddress);
        }

	signal(sig, SIG_IGN);

        close(file);
        exit(0);
}

//int main(int argc, char *argv[])
void led8x8(char **argv)
{
        struct timespec tim, tim2;
        tim.tv_sec=1;
        tim2.tv_nsec=500000000L; // .5 second
        // char *argv="test";

        char *end;
        int count,cont;
        char filename[20];
        unsigned char letter;
        int i,t,y;

        i2cbus   = 1;
        address  = 112;
        daddress = 0;
	char text[strlen(argv[0])+4];

	signal(SIGINT, INThandler);

	//Startup the matrix
	size = I2C_SMBUS_BYTE;
	sprintf(filename, "/dev/i2c-%d", i2cbus);
	file = open(filename, O_RDWR);
	if (file<0) {
		if (errno == ENOENT) {
			fprintf(stderr, "Error: Could not open file "
				"/dev/i2c-%d: %s\n", i2cbus, strerror(ENOENT));
			}
		 else {
			fprintf(stderr, "Error: Could not open file "

				"`%s': %s\n", filename, strerror(errno));
			if (errno == EACCES)
				fprintf(stderr, "Run as root?\n");
		}
		exit(1);
	}

	if (ioctl(file, I2C_SLAVE, address) < 0) {
		fprintf(stderr,
			"Error: Could not set address to 0x%02x: %s\n",
			address, strerror(errno));
		exit(1);
	}

	res = i2c_smbus_write_byte(file, daddress);
	if (res < 0) {
		fprintf(stderr, "Warning - write failed, filename=%s, daddress=%d\n",
			filename, daddress);
	}

	daddress = 0x21; // Start oscillator (page 10)
	//printf("writing: 0x%02x\n", daddress);
	res = i2c_smbus_write_byte(file, daddress);

	daddress = 0x81; // Display on, blinking off (page 11)
	//printf("writing: 0x%02x\n", daddress);
	res = i2c_smbus_write_byte(file, daddress);

	daddress = 0xef; // Full brightness (page 15)
	//printf("Full brightness writing: 0x%02x\n", daddress);
	res = i2c_smbus_write_byte(file, daddress);

	daddress = 0x00; // Start writing to address 0 (page 13)
	//printf("Start writing to address 0 writing: 0x%02x\n", daddress);
	res = i2c_smbus_write_byte(file, daddress);


	//Setup the text  argument that was passed to main. remove null and add some extra spaces.

        for(i = 0; i < (strlen(argv[0])) ; i++){
	        text[i] = argv[0][i];
        }
        for(i = 0; i < 4 ; i++){
                text[strlen(argv[0])+i] = 32;
        }

        //put all the characters of the scrolling text in a contiguous block

	int length = (strlen(text))-2;
	int Vposition,c,character, l;
	unsigned short int   displayBuffer[8][length*8];
	unsigned short int   display[8];
	for(i = 0; i < length ; i++){
		character = (text[i]-31);
		 for(Vposition = 0; Vposition < 8 ; Vposition++){
			displayBuffer[Vposition][i] = FONT8x8[character][Vposition];
		}
	}

	//Text scrolling happens here
	unsigned short int   bitShifted[8];
	for(letter=0; letter<(length-1); letter++){
        	for(y=0; y<8; y++){
	  		for(i=0; i<8; i++){
				bitShifted[i] = (displayBuffer[i][letter]) << y | (displayBuffer[i][letter+1]) >> (8-y);
				bitShifted[i]  = reverseBits(bitShifted[i]);
			}
			displayImage(bitShifted,res, daddress, file);
		    		//for(i=0; i<8; i++){
			        //	printbitssimple( bitShifted[i]);
				//}
		}
                nanosleep(&tim, &tim2);
	}

}

