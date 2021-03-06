/*
 * Software UART implementation for the ESP8266.
 * 
 * @author Michel Megens
 * @author Ruslan V. Uss
 * @author Bernhard Guillon
 * @email  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <lwiot/log.h>
#include <lwiot/uart.h>
#include <lwiot/types.h>

#include <esp/gpio.h>
#include <espressif/esp_common.h>

#include "esp8266-softuart.h"

#ifdef SOFTUART_DEBUG
#define debug(fmt, ...) printf("[SOFT-UART]: " fmt, ## __VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#ifndef SOFTUART_MAX_RX_BUFF
#define SOFTUART_MAX_RX_BUFF 64
#endif

#define MAX_SOFTUART 2

typedef struct {
	char receive_buffer[SOFTUART_MAX_RX_BUFF];
	uint8_t receive_buffer_tail;
	uint8_t receive_buffer_head;
	uint8_t buffer_overflow;
} softuart_buffer_t;

typedef struct {
    uint8_t rx_pin, tx_pin;
    uint32_t baudrate;
    volatile softuart_buffer_t buffer;
    uint16_t bit_time;
} softuart_t;

static softuart_t uarts[MAX_SOFTUART] = { { 0 } };

#ifndef SOFTUART_MAX_UARTS
#define SOFTUART_MAX_UARTS MAX_SOFTUART
#endif

static inline int8_t find_uart_by_rx(uint8_t rx_pin)
{
    for (uint8_t i = 0; i < SOFTUART_MAX_UARTS; i++)
        if (uarts[i].baudrate && uarts[i].rx_pin == rx_pin)
			return i;

    return -1;
}

static void handle_rx(uint8_t gpio_num)
{
    int8_t uart_no = find_uart_by_rx(gpio_num);

    if (uart_no < 0)
		return;

    softuart_t *uart = uarts + uart_no;
    gpio_set_interrupt(gpio_num, GPIO_INTTYPE_NONE, handle_rx);
    // Wait till start bit is half over so we can sample the next one in the center
    sdk_os_delay_us(uart->bit_time / 2);

    // Now sample bits
    uint8_t d = 0;
    uint32_t start_time = 0x7FFFFFFF & sdk_system_get_time();

    for (uint8_t i = 0; i < 8; i++)
    {
        while ((0x7FFFFFFF & sdk_system_get_time()) < (start_time + (uart->bit_time * (i + 1))))
        {
            // If system timer overflow, escape from while loop
            if ((0x7FFFFFFF & sdk_system_get_time()) < start_time)
                break;
        }
        // Shift d to the right
        d >>= 1;

        // Read bit
        if (gpio_read(uart->rx_pin))
        {
            // If high, set msb of 8bit to 1
            d |= 0x80;
        }
    }

    uint8_t next = (uart->buffer.receive_buffer_tail + 1) % SOFTUART_MAX_RX_BUFF;
    if (next != uart->buffer.receive_buffer_head) {
        // save new data in buffer: tail points to where byte goes
        uart->buffer.receive_buffer[uart->buffer.receive_buffer_tail] = d; // save new byte
        uart->buffer.receive_buffer_tail = next;
    } else {
        uart->buffer.buffer_overflow = 1;
    }

    // Wait for stop bit
    sdk_os_delay_us(uart->bit_time);
    gpio_set_interrupt(uart->rx_pin, GPIO_INTTYPE_EDGE_NEG, handle_rx);
}

static bool check_uart_no(uint8_t uart_no)
{
    if (uart_no >= SOFTUART_MAX_UARTS)
    {
        debug("Invalid uart number %d, %d max", uart_no, SOFTUART_MAX_UARTS);
        return false;
    }

    return true;
}

static bool check_uart_enabled(uint8_t uart_no)
{
    if (!uarts[uart_no].baudrate)
    {
        debug("Uart %d is disabled", uart_no);
        return false;
    }

    return true;
}

bool esp_softuart_open(uint8_t uart_no, uint32_t baudrate, uint8_t rx_pin, uint8_t tx_pin)
{
    if (!check_uart_no(uart_no))
		return false;
    if (baudrate == 0) {
        debug("Invalid baudrate");
        return false;
    }

    for (uint8_t i = 0; i < SOFTUART_MAX_UARTS; i++)
        if (uarts[i].baudrate && i != uart_no &&
			(uarts[i].rx_pin == rx_pin || uarts[i].tx_pin == tx_pin ||
			uarts[i].rx_pin == tx_pin || uarts[i].tx_pin == rx_pin)) {
            debug("Cannot share pins between uarts");
            return false;
        }

    esp_softuart_close(uart_no);

    softuart_t *uart = uarts + uart_no;

    uart->baudrate = baudrate;
    uart->rx_pin = rx_pin;
    uart->tx_pin = tx_pin;

    // Calculate bit_time
    uart->bit_time = (1000000 / baudrate);
    if (((100000000 / baudrate) - (100 * uart->bit_time)) > 50)
		uart->bit_time++;

    // Setup Rx
    gpio_enable(rx_pin, GPIO_INPUT);
    gpio_set_pullup(rx_pin, true, false);

    // Setup Tx
    gpio_enable(tx_pin, GPIO_OUTPUT);
    gpio_set_pullup(tx_pin, true, false);
    gpio_write(tx_pin, 1);

    // Setup the interrupt handler to get the start bit
    gpio_set_interrupt(rx_pin, GPIO_INTTYPE_EDGE_NEG, handle_rx);

    sdk_os_delay_us(1000); // TODO: not sure if it really needed

    return true;
}

bool esp_softuart_close(uint8_t uart_no)
{
    if (!check_uart_no(uart_no))
		return false;

    softuart_t *uart = uarts + uart_no;

    if (!uart->baudrate)
		return true;

    gpio_set_interrupt(uart->rx_pin, GPIO_INTTYPE_NONE, NULL);
    uart->baudrate = 0;

    return true;
}

bool esp_softuart_put(uint8_t uart_no, char c)
{
    if (!check_uart_no(uart_no)) return false;
    if (!check_uart_enabled(uart_no)) return false;
    softuart_t *uart = uarts + uart_no;

    uint32_t start_time = 0x7FFFFFFF & sdk_system_get_time();
    gpio_write(uart->tx_pin, 0);

    for (uint8_t i = 0; i <= 8; i++) {
        while ((0x7FFFFFFF & sdk_system_get_time()) < (start_time + (uart->bit_time * (i + 1)))) {
            if ((0x7FFFFFFF & sdk_system_get_time()) < start_time)
                break;
        }
        gpio_write(uart->tx_pin, c & (1 << i));
    }

    while ((0x7FFFFFFF & sdk_system_get_time()) < (start_time + (uart->bit_time * 9))) {
        if ((0x7FFFFFFF & sdk_system_get_time()) < start_time)
            break;
    }

    gpio_write(uart->tx_pin, 1);
    sdk_os_delay_us(uart->bit_time * 6);

    return true;
}

bool esp_softuart_puts(uint8_t uart_no, const char *s)
{
    while (*s)
    {
        if (!esp_softuart_put(uart_no, *s++))
            return false;
    }

    return true;
}

bool esp_softuart_available(uint8_t uart_no)
{
    if (!check_uart_no(uart_no))
		return false;
    if (!check_uart_enabled(uart_no))
		return false;

    softuart_t *uart = uarts + uart_no;
    return (uart->buffer.receive_buffer_tail + SOFTUART_MAX_RX_BUFF - uart->buffer.receive_buffer_head) % SOFTUART_MAX_RX_BUFF;
}

uint8_t esp_softuart_read(uint8_t uart_no)
{
    if (!check_uart_no(uart_no))
		return 0;
    if (!check_uart_enabled(uart_no))
		return 0;
    softuart_t *uart = uarts + uart_no;

    // Empty buffer?
    if (uart->buffer.receive_buffer_head == uart->buffer.receive_buffer_tail) return 0;

    // Read from "head"
    uint8_t d = uart->buffer.receive_buffer[uart->buffer.receive_buffer_head];
    uart->buffer.receive_buffer_head = (uart->buffer.receive_buffer_head + 1) % SOFTUART_MAX_RX_BUFF;
    return d;
}
