#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

struct GPIO
{
	void * gpio_map = nullptr;
	volatile uint32_t * gpio = nullptr;
	
	bool init();
	void shut();
	
	void setInput(const int index);
	void setOutput(const int index);
	
	void set(const uint32_t mask); // sets bits which are 1 ignores bits which are 0
	void unset(const uint32_t mask); // clears bits which are 1 ignores bits which are 0
};

#define BCM2708_PERI_BASE 0x3F000000
#define GPIO_BASE (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GET_GPIO(g) (*(gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH

#define GPIO_PULL *(gpio+37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio+38) // Pull up/pull down clock

int main(int argc, char **argv)
{
	GPIO gpio;
	
	if (!gpio.init())
	{
		printf("GPIO init failed\n");
		return -1;
	}

	// set GPIO pins 7-11 to output
	for (int g = 7; g <= 11; ++g)
	{
		gpio.setOutput(g);
	}

	int n[12];

	for (int i = 0; i < 12; ++i)
		n[i] = 1 + (rand() % 20);

	for (int r = 0; r < 2000000000; ++r)
	{
		if ((r % 40000) == 0)
		{
			for (int i = 0; i < 12; ++i)
				 n[i]++;
		}

		for (int i = 0; i < 12; ++i)
		{
			if (n[i] == 30)
				n[i] = 1;
		}

		int set = 0;
		int unset = 0;

		for (int i = 7; i < 12; ++i)
		{
			const int v = (r % n[i]) == 0;

			if (v)
				set |= 1 << i;
			else
				unset |= 1 << i;
		}
		
		gpio.set(set);
		gpio.unset(unset);
	}
	
	gpio.shut();

	return 0;
}

//
// Set up a memory regions to access GPIO
//

bool GPIO::init()
{
	int mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
	
	if (mem_fd < 0)
	{
		return false;
	}

	/* mmap GPIO */
	gpio_map = mmap(
		NULL,             //Any adddress in our space will do
		BLOCK_SIZE,       //Map length
		PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
		MAP_SHARED,       //Shared with other processes
		mem_fd,           //File to map
		GPIO_BASE         //Offset to GPIO peripheral
	);

	close(mem_fd); //No need to keep mem_fd open after mmap
	mem_fd = -1;

	if (gpio_map == MAP_FAILED)
	{
		return false;
	}
	
	gpio = (volatile unsigned *)gpio_map;
	
	return true;
}

void GPIO::shut()
{
	// todo : release memory mapped memory
}

void GPIO::setInput(const int index)
{
	INP_GPIO(index);
}

void GPIO::setOutput(const int index)
{
	INP_GPIO(index); // must use INP_GPIO before we can use OUT_GPIO
	OUT_GPIO(index);
}

void GPIO::set(const uint32_t mask)
{
	gpio[7] = mask;
}

void GPIO::unset(const uint32_t mask)
{
	gpio[10] = mask;
}
