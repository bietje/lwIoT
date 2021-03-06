#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <esp_attr.h>
#include <lwiot.h>

#include <lwiot/thread.h>
#include <lwiot/string.h>
#include <lwiot/uart.h>
#include <lwiot/string.h>
#include <lwiot/esp32/esp32uart.h>

class MainThread : public lwiot::Thread {
public:
	explicit MainThread(const char *arg) : Thread("main-thread", (void*)arg)
	{ }

protected:
	void run(void *arg)
	{
		char *buffer = (char*)malloc(32);
		lwiot::esp32::Uart uart(2, 9600);
		lwiot::GpioPin out = 33;
		uint32_t data = 0xDEADBEEF;

		printf("Main thread started!\n");
		while(true) {
			memset((void*)&buffer[0], 0, 32);
			uart.write((const uint8_t*)&data, sizeof(data));
			uart.read((uint8_t*)&buffer[0], 17);
			printf("Received: %s\n", buffer);
			lwiot_sleep(1000);
		}
	}
};

static MainThread *mt;
const char *arg = "Hello, World! [FROM main-thread]";

extern "C" void main_start(void)
{
	printf("Creating main thread..");
	mt = new MainThread(arg);
	printf("[DONE]\n");
	mt->start();
}

