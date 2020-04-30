/*
 * Motion_Sensor.h
 *
 * Created: 22.02.2016 1:30:48
 
 *  Author: isairon
 Писалась для LIS331DL
 Используется библиотека spi.c
 Инициализация должна быть проведена от неё. 
 
 */ 
#ifndef MOTION_SENSOR_H_
#define MOTION_SENSOR_H_

#define CS_PORT PORTB
#define CS_unaffected_bit PB1

#define TWI_addr 0x00111000 // Где последний-бит чтения/записи, а предпоследний -выбирается ножкой SD0|SA0
// биты идут по нисходящему (заднему)фронту SCK

// functions
uint8_t spiMEMS_ReadByte(uint8_t memAddr); // если не связываться с указателями.

extern void SPI_accelerometr_Write(uint8_t Addr, uint8_t data);
extern void SPI_accelerometr_Read(uint8_t Addr, uint8_t *reg);
extern void SPI_accelerometr_Read_char(uint8_t Addr, signed char *reg);
uint8_t LIS_read__who_i_am (void);
void LIS_read__who_i_am_1 (uint8_t * dataout);
void LIS_read__who_i_am_2(uint8_t * dataout);
void LIS_read__who_i_am_3 (uint8_t * dataout);
#endif /* MOTION_SENSOR_H_ */

#define MemsWHO_AM_I 0x0F
#define MemsOUT_X 0x29
#define MemsOUT_Y 0x2B
#define MemsOUT_Z 0x2D
#define MemsCTRL_REG1 0x20 
#define MemsCTRL_REG2   0x21 
#define MemsCTRL_REG3   0x22 
#define MemsHP_FILTER_RESET   0x23 
#define MemsSTATUS_REG 0x27 
