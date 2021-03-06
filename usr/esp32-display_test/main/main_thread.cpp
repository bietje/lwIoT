/*
 * ESP32 device test.
 * 
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <esp_attr.h>
#include <esp_heap_caps.h>
#include <lwiot.h>

#include <lwiot/thread.h>
#include <lwiot/log.h>
#include <lwiot/string.h>
#include <lwiot/gpiochip.h>
#include <lwiot/gpiopin.h>
#include <lwiot/watchdog.h>
#include <lwiot/datetime.h>
#include <lwiot/wifiaccesspoint.h>
#include <lwiot/i2cbus.h>
#include <lwiot/softwarei2calgorithm.h>
#include <lwiot/ssd1306display.h>

#include <lwiot/esp32/esp32pwm.h>

class MainThread : public lwiot::Thread {
public:
	explicit MainThread(const char *arg) : Thread("main-thread", (void *) arg)
	{
	}

protected:
	void startPwm(lwiot::esp32::PwmTimer &timer)
	{
		auto &channel = timer[0];
		auto pin = lwiot::GpioPin(5);

		channel.setGpioPin(pin);
		channel.setDutyCycle(75.0f);
		channel.enable();
		lwiot_sleep(2000);

		channel.setDutyCycle(50.0f);
		timer.setFrequency(100);
		channel.reload();
	}

	void startAP(const lwiot::String &ssid, const lwiot::String &passw)
	{
		auto &ap = lwiot::WifiAccessPoint::instance();
		lwiot::IPAddress local(192, 168, 1, 1);
		lwiot::IPAddress subnet(255, 255, 255, 0);
		lwiot::IPAddress gw(192, 168, 1, 1);

		ap.config(local, gw, subnet);
		ap.begin(ssid, passw, 4);
	}

	void drawLines(lwiot::Ssd1306Display& display)
	{
		print_dbg("Width / Height: %i / %i\n", display.width(), display.height());

		for(int16_t i = 0; i < display.width(); i += 4) {
			display.drawLine(0, 0, i, display.height() - 1, WHITE);
			display.display();
			lwiot_sleep(1);
		}

		wdt.reset();

		for(int16_t i = 0; i < display.height(); i += 4) {
			display.drawLine(0, 0, display.width() - 1, i, WHITE);
			display.display();
			lwiot_sleep(1);
		}

		lwiot_sleep(250);
		wdt.reset();

		display.clear();
		for(int16_t i = 0; i < display.width(); i += 4) {
			display.drawLine(0, display.height() - 1, i, 0, WHITE);
			display.display();
			lwiot_sleep(1);
		}

		wdt.reset();

		for(int16_t i = display.height() - 1; i >= 0; i -= 4) {
			display.drawLine(0, display.height() - 1, display.width() - 1, i, WHITE);
			display.display();
			lwiot_sleep(1);
		}
		lwiot_sleep(250);
		wdt.reset();

		display.clear();
		for(int16_t i = display.width() - 1; i >= 0; i -= 4) {
			display.drawLine(display.width() - 1, display.height() - 1, i, 0, WHITE);
			display.display();
			lwiot_sleep(1);
		}

		wdt.reset();

		for(int16_t i = display.height() - 1; i >= 0; i -= 4) {
			display.drawLine(display.width() - 1, display.height() - 1, 0, i, WHITE);
			display.display();
			lwiot_sleep(1);
		}

		lwiot_sleep(250);
		display.clear();
		wdt.reset();

		for(int16_t i = 0; i < display.height(); i += 4) {
			display.drawLine(display.width() - 1, 0, 0, i, WHITE);
			display.display();
			lwiot_sleep(1);
		}
		wdt.reset();

		for(int16_t i = 0; i < display.width(); i += 4) {
			display.drawLine(display.width() - 1, 0, i, display.height() - 1, WHITE);
			display.display();
			lwiot_sleep(1);
		}

		lwiot_sleep(250);
		wdt.reset();
	}

	void testDisplay(lwiot::Ssd1306Display &display)
	{
		print_dbg("Starting display..\n");
		display.begin();
		display.display();

		lwiot_sleep(1000);
		wdt.reset();
		display.clear();
	}

	virtual void run(void *arg) override
	{
		size_t freesize;
		auto algo = new lwiot::SoftwareI2CAlgorithm(23, 22, 100000U);
		lwiot::I2CBus bus(algo);
		lwiot::Ssd1306Display display(bus);

		if(!algo->test()) {
			print_dbg("I2C self-test failed!\n");
		}

		printf("Main thread started!\n");
		wdt.enable(5000);

		freesize = heap_caps_get_free_size(0);
		print_dbg("Free heap size: %u\n", freesize);
		
		this->startAP("lwIoT test", "testap1234");
		wdt.reset();

		this->testDisplay(display);
		this->drawLines(display);
		display.setRotation(0);
		display.clear();
		display.setTextSize(1);
		display.setTextColor(WHITE);
		display.setTextWrap(false);
		display.setCursor(0, 3);

		display.print("Hello, World!");
		display.display();

		while(true) {
			print_dbg("MT ping\n");
			wdt.reset();
			lwiot_sleep(1000);
		}
	}
};

static MainThread *mt;
static const char *arg = "Hello, World! [FROM main-thread]";

extern "C" void main_start(void)
{
	printf("Creating main thread..");
	mt = new MainThread(arg);
	printf(" [DONE]\n");
	mt->start();
}

