#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
//#include <linux/i2c.h>
#include <linux/i2c-dev.h>

// set the following to the detected i2c address for the chip

#define CHIP_ADDRESS 0x77
#define CHIP_DEVICE "/dev/i2c-1"

// adapted from:
//     http://www.raspberrypi.org/forums/viewtopic.php?t=16968&p=177210
// removed the need for smbus.c and .h

// make sure that you've run apt-get install libi2c-dev
// compile with gcc -o temp temp.c
// to do: remove need for libi2c-dev

int open_chip() {
	int fd;
	char *filename = CHIP_DEVICE;

	//open port for reading and writing
	if((fd = open(filename, O_RDWR)) < 0)
		exit(1);

	// set port optoions and set address of device
	if(ioctl(fd, I2C_SLAVE, CHIP_ADDRESS) < 0) {
		close(fd);
		exit(1);
	}
	return fd;
}

int read_value(int fd, unsigned int address) {
	int r;

	r=i2c_smbus_read_word_data(fd,address);
	r=((r<<8) & 0xFF00)|((r>>8) & 0xFF);
	return r;
}

void write_to_chip(int fd, unsigned int address, unsigned int value) {
	i2c_smbus_write_byte_data(fd, address, value);
}

// you must tell the chip you want a temperature reading
// do this by sending 0x2E to register 0xF4 and then
// reading register 0xF6

unsigned int read_uncalibrated_temp_from_chip() {
	int fd=open_chip();
	write_to_chip(fd,0XF4,0x2E);
	usleep(5000);
	unsigned int ut=read_value(fd,0xF6);
	close(fd);
	return ut;
}

// following reads specific factory calibration values
// from the chip, and uses uncalibrated_temp_from_chip
// to calculate the temp in F (note: original vendor 
// formula output in units of 0.1 *C)

unsigned int calculate_temperature(unsigned int ut) {

	int x1, x2, b5;

	// read calibration values from chip

	int fd=open_chip();
	unsigned short int ac5=read_value(fd,0xB2);
	unsigned short int ac6=read_value(fd,0xB4);
	short int mc=read_value(fd,0xBC);
	short int md=read_value(fd,0xBE);
	close(fd);

	// calculate temperature using above

	x1 = (((int)ut - (int)ac6)*(int)ac5) >> 15;
	x2 = ((int)mc << 11)/(x1 + md);
	b5 = x1 + x2;
	
	unsigned int result = ((b5 + 8)>>4);
	
	return result;
}

int main( int argc, char *argv[]) {

	unsigned int temperature = calculate_temperature(read_uncalibrated_temp_from_chip());
	printf("Temperature\t%0.1f *F\n",((double)temperature)/10*1.8+32);

	return(0);
}
