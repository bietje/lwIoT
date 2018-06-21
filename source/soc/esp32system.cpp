/*
 * SoC system interface class definition.
 *
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>

#include <lwiot/log.h>
#include <lwiot/lwiot.h>
#include <lwiot/system.h>
#include <lwiot/esp32system.h>

#include <soc/soc.h>
#include <esp_sleep.h>

namespace lwiot
{
	Esp32System::Esp32System(SleepState state) : System(state)
	{
	}

	void Esp32System::sleep(const uint32_t& ms) const
	{
		uint64_t us;

		switch(this->sleepState()) {
		case SLEEP_DEEP:
		default:
			us = ms * 1000ULL;
			esp_deep_sleep(us);
			break;
		}
	}

	void Esp32System::restart() const
	{
		esp_restart();
	}
}