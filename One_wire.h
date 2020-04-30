/*
 * One_wire.h
 *
 * Created: 21.02.2015 22:32:40
 *  Author: idopshik
 */ 

#ifndef ONE_WIRE_H_
#define ONE_WIRE_H_

uint8_t onewire_level();
void onewire_strong_enable();
void onewire_strong_disable();
uint8_t onewire_reset();
void onewire_send_bit(uint8_t bit);
void onewire_send(uint8_t b);
uint8_t onewire_read_bit();
uint8_t onewire_read();
uint8_t onewire_crc_update(uint8_t crc, uint8_t b);
uint8_t onewire_skip();
uint8_t onewire_read_rom(uint8_t * buf);
uint8_t onewire_match(uint8_t * data);
void onewire_enum_init();
uint8_t * onewire_enum_next();
uint16_t uart_digit(uint16_t dig, uint16_t sub);
void uart_num(int16_t num);
void ONE_WIRE_DO_IT(void);
void send_skip_rom(void);	

#endif /* ONE_WIRE_H_ */
