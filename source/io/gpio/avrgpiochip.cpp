/*
 * AVR ATmega GPIO implementation.
 *
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include <stdlib.h>
#include <lwiot.h>

#include <lwiot/types.h>
#include <lwiot/gpiochip.h>
#include <lwiot/avrgpiochip.h>

extern const uint16_t PROGMEM port_to_mode_PGM[];
extern const uint16_t PROGMEM port_to_input_PGM[];
extern const uint16_t PROGMEM port_to_output_PGM[];
extern const uint8_t PROGMEM digital_pin_to_port_PGM[];
extern const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[];

#define digitalPinToPort(P) ( pgm_read_byte( digital_pin_to_port_PGM + (P) ) )
#define digitalPinToBitMask(P) ( pgm_read_byte( digital_pin_to_bit_mask_PGM + (P) ) )

#define portOutputRegister(P) ( (volatile uint8_t *)( pgm_read_word( port_to_output_PGM + (P))) )
#define portInputRegister(P) ( (volatile uint8_t *)( pgm_read_word( port_to_input_PGM + (P))) )
#define portModeRegister(P) ( (volatile uint8_t *)( pgm_read_word( port_to_mode_PGM + (P))) )

static volatile irq_handler_t handlers[EXTERNAL_NUM_INTERRUPTS] = {0,};

#ifndef IMPLEMENT_ISR
#define IMPLEMENT_ISR(vect, interrupt) \
  ISR(vect) { \
    irq_handler_t handler = handlers[interrupt]; \
	if(handler == NULL) \
		return; \
	handler(); \
  }
#endif

extern "C" {

#if defined(__AVR_ATmega32U4__)

IMPLEMENT_ISR(INT0_vect, EXTERNAL_INT_0)
IMPLEMENT_ISR(INT1_vect, EXTERNAL_INT_1)
IMPLEMENT_ISR(INT2_vect, EXTERNAL_INT_2)
IMPLEMENT_ISR(INT3_vect, EXTERNAL_INT_3)
IMPLEMENT_ISR(INT6_vect, EXTERNAL_INT_4)

#elif defined(EICRA) && defined(EICRB)

IMPLEMENT_ISR(INT0_vect, EXTERNAL_INT_2)
IMPLEMENT_ISR(INT1_vect, EXTERNAL_INT_3)
IMPLEMENT_ISR(INT2_vect, EXTERNAL_INT_4)
IMPLEMENT_ISR(INT3_vect, EXTERNAL_INT_5)
IMPLEMENT_ISR(INT4_vect, EXTERNAL_INT_0)
IMPLEMENT_ISR(INT5_vect, EXTERNAL_INT_1)
IMPLEMENT_ISR(INT6_vect, EXTERNAL_INT_6)
IMPLEMENT_ISR(INT7_vect, EXTERNAL_INT_7)

#else

IMPLEMENT_ISR(INT0_vect, EXTERNAL_INT_0)
IMPLEMENT_ISR(INT1_vect, EXTERNAL_INT_1)

#if defined(EICRA) && defined(ISC20)
IMPLEMENT_ISR(INT2_vect, EXTERNAL_INT_2)
#endif

#endif
}

namespace lwiot
{
	AvrGpioChip::AvrGpioChip() : GpioChip(PINS)
	{ }

	void AvrGpioChip::mode(int pin, const PinMode& mode)
	{
		uint8_t bit, port;
		uint8_t old;
		volatile uint8_t *reg, *out;

		bit = digitalPinToBitMask(pin);
		port = digitalPinToPort(pin);

		if(port == NOT_A_PIN)
			return;

		reg = portModeRegister(port);
		out = portOutputRegister(port);

		old = SREG;
		cli();

		switch(mode) {
		case INPUT:
			*reg &= ~bit;
			*out &= ~bit;
			break;

		case INPUT_NOPULLUP:
			*reg &= ~bit;
			*out &= ~bit;
			break;

		case INPUT_PULLUP:
			*reg &= ~bit;
			*out |= bit;
			break;

		case OUTPUT_OPEN_DRAIN:
			*out &= ~bit;
		case OUTPUT:
			*reg |= bit;
			break;

		default:
			break;
		}

		SREG = old;
	}

	void AvrGpioChip::write(int pin, bool value)
	{
		uint8_t bit, port;
		uint8_t old;
		volatile uint8_t *out;

		bit = digitalPinToBitMask(pin);
		port = digitalPinToPort(pin);

		if(port == NOT_A_PIN)
			return;

		out = portOutputRegister(port);

		old = SREG;
		cli();

		if(value)
			*out &= ~bit;
		else
			*out |= bit;

		SREG = old;
	}

	bool AvrGpioChip::read(int pin)
	{
		uint8_t bit, port;
		uint8_t old;
		volatile uint8_t *in;

		bit = digitalPinToBitMask(pin);
		port = digitalPinToPort(pin);

		if(port == NOT_A_PIN)
			return false;

		in = portInputRegister(port);

		old = SREG;
		cli();
		auto rv = *in & bit;
		SREG = old;

		return !!rv;
	}

	void AvrGpioChip::setOpenDrain(int pin)
	{
		this->input(pin);
		this->write(pin, false);
	}

	void AvrGpioChip::odWrite(int pin, bool value)
	{
		if(value)
			this->input(pin);
		else
			this->output(pin);
	}

	void AvrGpioChip::attachIrqHandler(int pin, irq_handler_t handler, IrqEdge edge)
	{
		auto mode = this->mapIrqEdge(edge);

		if(mode < 0)
			return;

		if(pin < EXTERNAL_NUM_INTERRUPTS) {
			handlers[pin] = handler;
			
			// Configure the interrupt mode (trigger on low input, any change, rising
			// edge, or falling edge).  The mode constants were chosen to correspond
			// to the configuration bits in the hardware register, so we simply shift
			// the mode into place.
			
			// Enable the interrupt.
			
			switch (pin) {
#if defined(__AVR_ATmega32U4__)
			// I hate doing this, but the register assignment differs between the 1280/2560
			// and the 32U4.  Since avrlib defines registers PCMSK1 and PCMSK2 that aren't 
			// even present on the 32U4 this is the only way to distinguish between them.
			case 0:
				EICRA = (EICRA & ~((1<<ISC00) | (1<<ISC01))) | (mode << ISC00);
				EIMSK |= (1<<INT0);
				break;

			case 1:
				EICRA = (EICRA & ~((1<<ISC10) | (1<<ISC11))) | (mode << ISC10);
				EIMSK |= (1<<INT1);
				break;	

			case 2:
				EICRA = (EICRA & ~((1<<ISC20) | (1<<ISC21))) | (mode << ISC20);
				EIMSK |= (1<<INT2);
				break;

			case 3:
				EICRA = (EICRA & ~((1<<ISC30) | (1<<ISC31))) | (mode << ISC30);
				EIMSK |= (1<<INT3);
				break;

			case 4:
				EICRB = (EICRB & ~((1<<ISC60) | (1<<ISC61))) | (mode << ISC60);
				EIMSK |= (1<<INT6);
				break;
		#elif defined(EICRA) && defined(EICRB) && defined(EIMSK)
			case 2:
				EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01))) | (mode << ISC00);
				EIMSK |= (1 << INT0);
				break;

			case 3:
				EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11))) | (mode << ISC10);
				EIMSK |= (1 << INT1);
				break;

			case 4:
				EICRA = (EICRA & ~((1 << ISC20) | (1 << ISC21))) | (mode << ISC20);
				EIMSK |= (1 << INT2);
				break;

			case 5:
				EICRA = (EICRA & ~((1 << ISC30) | (1 << ISC31))) | (mode << ISC30);
				EIMSK |= (1 << INT3);
				break;

			case 0:
				EICRB = (EICRB & ~((1 << ISC40) | (1 << ISC41))) | (mode << ISC40);
				EIMSK |= (1 << INT4);
				break;

			case 1:
				EICRB = (EICRB & ~((1 << ISC50) | (1 << ISC51))) | (mode << ISC50);
				EIMSK |= (1 << INT5);
				break;

			case 6:
				EICRB = (EICRB & ~((1 << ISC60) | (1 << ISC61))) | (mode << ISC60);
				EIMSK |= (1 << INT6);
				break;

			case 7:
				EICRB = (EICRB & ~((1 << ISC70) | (1 << ISC71))) | (mode << ISC70);
				EIMSK |= (1 << INT7);
				break;
		#else		
			case 0:
#if defined(EICRA) && defined(ISC00) && defined(EIMSK)
				EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01))) | (mode << ISC00);
				EIMSK |= (1 << INT0);
#elif defined(MCUCR) && defined(ISC00) && defined(GICR)
				MCUCR = (MCUCR & ~((1 << ISC00) | (1 << ISC01))) | (mode << ISC00);
				GICR |= (1 << INT0);
#elif defined(MCUCR) && defined(ISC00) && defined(GIMSK)
				MCUCR = (MCUCR & ~((1 << ISC00) | (1 << ISC01))) | (mode << ISC00);
				GIMSK |= (1 << INT0);
#else
#error attachInterrupt not finished for this CPU (case 0)
#endif
				break;

			case 1:
#if defined(EICRA) && defined(ISC10) && defined(ISC11) && defined(EIMSK)
				EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11))) | (mode << ISC10);
				EIMSK |= (1 << INT1);
#elif defined(MCUCR) && defined(ISC10) && defined(ISC11) && defined(GICR)
				MCUCR = (MCUCR & ~((1 << ISC10) | (1 << ISC11))) | (mode << ISC10);
				GICR |= (1 << INT1);
#elif defined(MCUCR) && defined(ISC10) && defined(GIMSK) && defined(GIMSK)
				MCUCR = (MCUCR & ~((1 << ISC10) | (1 << ISC11))) | (mode << ISC10);
				GIMSK |= (1 << INT1);
#else
#warning attachInterrupt may need some more work for this cpu (case 1)
#endif
				break;
			
			case 2:
#if defined(EICRA) && defined(ISC20) && defined(ISC21) && defined(EIMSK)
				EICRA = (EICRA & ~((1 << ISC20) | (1 << ISC21))) | (mode << ISC20);
				EIMSK |= (1 << INT2);
#elif defined(MCUCR) && defined(ISC20) && defined(ISC21) && defined(GICR)
				MCUCR = (MCUCR & ~((1 << ISC20) | (1 << ISC21))) | (mode << ISC20);
				GICR |= (1 << INT2);
#elif defined(MCUCR) && defined(ISC20) && defined(GIMSK) && defined(GIMSK)
				MCUCR = (MCUCR & ~((1 << ISC20) | (1 << ISC21))) | (mode << ISC20);
				GIMSK |= (1 << INT2);
#endif
				break;
#endif
			}
		}
	}

#define CHANGE 1
#define FALLING 2
#define RISING 3

	int AvrGpioChip::mapIrqEdge(const IrqEdge& edge) const
	{
		switch(edge) {
		case IrqRising:
			return RISING;

		case IrqFalling:
			return FALLING;

		case IrqRisingFalling:
			return CHANGE;

		default:
			return -1;
		}
	}
}

static lwiot::AvrGpioChip avr_gpio;
lwiot::GpioChip& gpio = avr_gpio;
