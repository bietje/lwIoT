/*
 * ESP32 I2C algorithm implementation.
 * 
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <lwiot.h>

#include <lwiot/bytebuffer.h>
#include <lwiot/log.h>
#include <lwiot/error.h>
#include <lwiot/types.h>
#include <lwiot/i2cmessage.h>
#include <lwiot/vector.h>
#include <lwiot/i2calgorithm.h>
#include <lwiot/gpiopin.h>

#include <lwiot/esp32/esp32i2calgorithm.h>

#include <driver/i2c.h>

#define ACK I2C_MASTER_ACK
#define NACK I2C_MASTER_LAST_NACK

#define TIMEOUT 500

namespace lwiot { namespace esp32
{
	I2CAlgorithm::I2CAlgorithm(int sda, int scl, uint32_t freq, i2c_port_t num) :
		lwiot::I2CAlgorithm(), _sda(sda), _scl(scl), _portno(num)
	{
		this->_sda.setOpenDrain();
		this->_scl.setOpenDrain();

		this->_frequency = freq;
		this->config.mode = I2C_MODE_MASTER;
		this->config.sda_io_num = static_cast<gpio_num_t>(sda);
		this->config.scl_io_num = static_cast<gpio_num_t>(scl);
		this->config.scl_pullup_en = GPIO_PULLUP_DISABLE;
		this->config.sda_pullup_en = GPIO_PULLUP_DISABLE;
		this->config.master.clk_speed = this->frequency();

		i2c_param_config(num, &this->config);
		i2c_driver_install(num, this->config.mode, 0, 0, 0);

		i2c_set_timeout(num, 12000);
	}

	I2CAlgorithm::I2CAlgorithm() : I2CAlgorithm(23, 22)
	{
	}

	I2CAlgorithm::~I2CAlgorithm()
	{
		i2c_driver_delete(this->_portno);

		this->_sda.input();
		this->_scl.input();
	}

	void I2CAlgorithm::setFrequency(const uint32_t& freq)
	{
		lwiot::I2CAlgorithm::setFrequency(freq);

		this->config.master.clk_speed = freq;
		i2c_param_config(this->_portno, &this->config);
	}

	ssize_t I2CAlgorithm::transfer(I2CMessage& msg)
	{
		auto handle = i2c_cmd_link_create();

		i2c_master_start(handle);
		this->prepareTransfer(handle, msg);
		i2c_master_stop(handle);
		auto err = i2c_master_cmd_begin(this->_portno, handle, TIMEOUT / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(handle);

		if(err != ESP_OK)
			return -EINVALID;

		return msg.count();
	}

#if 0
	ssize_t I2CAlgorithm::transfer(Vector<I2CMessage>& msgs)
	{
		auto handle = i2c_cmd_link_create();
		esp_err_t error;
		ssize_t total = 0L;

		i2c_master_start(handle);
		for(auto& msg : msgs) {
			this->prepareTransfer(handle, msg);

			if(msg.repstart())
				i2c_master_start(handle);
			else
				i2c_master_stop(handle);

			total += msg.count();
		}

		error = i2c_master_cmd_begin(this->_portno, handle, TIMEOUT / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(handle);

		return (error == ESP_OK) ? total : -EINVALID;
	}
#endif

	ssize_t I2CAlgorithm::transfer(Vector<I2CMessage*>& msgs)
	{
		i2c_cmd_handle_t handle;
		ssize_t total = 0L;

		for(auto& _msg : msgs) {
			auto& msg = *_msg;
			handle = i2c_cmd_link_create();
			i2c_master_start(handle);

			this->prepareTransfer(handle, msg);

			if(!msg.repstart())
				i2c_master_stop(handle);

			auto err = i2c_master_cmd_begin(this->_portno, handle, TIMEOUT / portTICK_PERIOD_MS);

			if(err != ESP_OK)
				total = -EINVALID;

			i2c_cmd_link_delete(handle);

			if(total >= 0)
				total += msg.count();
		}

		return total;
	}

	void I2CAlgorithm::prepareTransfer(i2c_cmd_handle_t handle, I2CMessage& msg) const
	{
		uint8_t *data;
		uint16_t address = msg.address() << 1;

		if(msg.isRead()) {
			data = msg.data();

			i2c_master_write_byte(
				handle,
				address | 1,
				true
			);

			auto idx = 0UL;
			for(; idx < msg.count() - 1; idx++) {
				i2c_master_read_byte(handle, &data[idx], ACK);
			}

			i2c_master_read_byte(handle, data + idx, I2C_MASTER_NACK);

			msg.setIndex(msg.count());
		} else {
			i2c_master_write_byte(
				handle,
				address,
				true
			);

			//i2c_master_write(handle, msg.data(), msg.count(), I2C_MASTER_ACK);

			for(auto idx = 0UL; idx < msg.count(); idx++) {
				i2c_master_write_byte(handle, msg[idx], true);
			}
		}
	}
}
}
