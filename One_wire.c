/*
 * One_wire.h
 *
 * Created: 11.08.2014 19:47:59
 *  Author: idopshik
 */ 

#include <avr/io.h>

#define F_CPU 12000000UL // Объявление частоты микроконтроллера для макросов _delay_us

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/interrupt.h>

extern void ReadedByteFromSlave(void);

char One_wire_buf[10]; // Если писать не с нулевого , то писать не будет - сразу выйдет по нулю. (первый нуль - то).
 
unsigned char do_1wire_or_nobody_home = 1;
unsigned char one_w_i = 0;


#define ONEWIRE_PORT PORTC
#define ONEWIRE_DDR DDRC
#define ONEWIRE_PIN PINC
#define ONEWIRE_PIN_NUM PC0

/*
const uint8_t PROGMEM onewire_crc_table[] = {
  0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41, 
  0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e, 0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc, 
  0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62, 
  0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff, 
  0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5, 0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07, 
  0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a, 
  0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24, 
  0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b, 0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9, 
  0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd, 
  0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50, 
  0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee, 
  0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73, 
  0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b, 
  0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4, 0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16, 
  0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8, 
  0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35  
};
*/

// Устанавливает низкий уровень на шине 1-wire
inline void onewire_low() {
  ONEWIRE_DDR |= _BV(ONEWIRE_PIN_NUM);
}

// "Отпускает" шину 1-wire
inline void onewire_high() {
  ONEWIRE_DDR &= ~_BV(ONEWIRE_PIN_NUM);
}

// Читает значение уровня на шине 1-wire
inline uint8_t onewire_level() {
  return ONEWIRE_PIN & _BV(ONEWIRE_PIN_NUM);
}


// Определения и функции ниже нужны только если требуется "сильный" подтягивающий резистор
#define ONEWIRE_STRONG_DDR DDRB
#define ONEWIRE_STRONG_PORT PORTB
#define ONEWIRE_STRONG_PIN_NUM PB1

// включение "сильной" подтяжки
void onewire_strong_enable() {
  // Для исключения низкого уровня на шине, сначала изменяется регистр значения
  ONEWIRE_STRONG_PORT |= _BV(ONEWIRE_STRONG_PIN_NUM);
  // затем - регистр направления
  ONEWIRE_STRONG_DDR |= _BV(ONEWIRE_STRONG_PIN_NUM); 
}

// отключение "сильной" подтяжки
void onewire_strong_disable() {
  // Для исключения низкого уровня на шине, сначала изменяется регистр направления
  ONEWIRE_STRONG_DDR &= ~_BV(ONEWIRE_STRONG_PIN_NUM); 
  // затем - регистр значения
  ONEWIRE_STRONG_PORT &= ~_BV(ONEWIRE_STRONG_PIN_NUM);
}


// Выдаёт импульс сброса (reset), ожидает ответный импульс присутствия.
// Если импульс присутствия получен, дожидается его завершения и возвращает 1, иначе возвращает 0 
uint8_t onewire_reset() 
{
  onewire_low();
  _delay_us(640); // Пауза 480..960 мкс
  onewire_high();
  _delay_us(2); // Время необходимое подтягивающему резистору, чтобы вернуть высокий уровень на шине
  // Ждём не менее 60 мс до появления импульса присутствия;
  for (uint8_t c = 80; c; c--) {
    if (!onewire_level()) {
      // Если обнаружен импульс присутствия, ждём его окончания
      while (!onewire_level()) { } // Ждём конца сигнала присутствия
      return 1;
    }
    _delay_us(1);
  }
  return 0;
}


// Отправляет один бит
// bit отправляемое значение, 0 - ноль, любое другое значение - единица
void onewire_send_bit(uint8_t bit) {
  onewire_low();
  if (bit) {
    _delay_us(5); // Низкий импульс, от 1 до 15 мкс (с учётом времени восстановления уровня)
    onewire_high();
    _delay_us(90); // Ожидание до завершения таймслота (не менее 60 мкс)
  } else {
    _delay_us(90); // Высокий уровень на весь таймслот (не менее 60 мкс, не более 120 мкс)
    onewire_high();
    _delay_us(5); // Время восстановления высокого уровеня на шине + 1 мс (минимум)
  }
}

// Отправляет один байт, восемь подряд бит, младший бит вперёд
// b - отправляемое значение
void onewire_send(uint8_t b) {
  for (uint8_t p = 8; p; p--) {
    onewire_send_bit(b & 1);
    b >>= 1;
  }
}


// читает значение бита, передаваемое уйстройством.
// Возвращает 0 - если передан 0, отличное от нуля значение - если передана единица
uint8_t onewire_read_bit() {
  onewire_low();
  _delay_us(2); // Длительность низкого уровня, минимум 1 мкс
  onewire_high();
  _delay_us(8); // Пауза до момента сэмплирования, всего не более 15 мкс
  uint8_t r = onewire_level();
  _delay_us(80); // Ожидание до следующего тайм-слота, минимум 60 мкс с начала низкого уровня
  return r;
}

// Читает один байт, переданный устройством, младший бит вперёд, возвращает прочитанное значение
uint8_t onewire_read() {
  uint8_t r = 0;
  for (uint8_t p = 8; p; p--) {
    r >>= 1;
    if (onewire_read_bit())
      r |= 0x80;
  }
  return r;
}


// Обновляет значение контольной суммы crc применением всех бит байта b.
// Возвращает обновлённое значение контрольной суммы
uint8_t onewire_crc_update(uint8_t crc, uint8_t b) 
{
  //return pgm_read_byte(&onewire_crc_table[crc ^ b]);
  
       for (uint8_t p = 8; p; p--) {
        crc = ((crc ^ b) & 1) ? (crc >> 1) ^ 0b10001100 : (crc >> 1);
         b >>= 1;
         }
         return crc;
}


// Выполняет последовательность инициализации (reset + ожидает импульс присутствия)
// Если импульс присутствия получен, выполняет команду SKIP ROM
// Возвращает 1, если импульс присутствия получен, 0 - если нет
uint8_t onewire_skip() {
  if (!onewire_reset())
    return 0;
  onewire_send(0xCC);
  return 1;
}


// Выполняет последовательность инициализации (reset + ожидает импульс присутствия)
// Если импульс присутствия получен, выполняет команду READ ROM, затем читает 8-байтовый код устройства
//    и сохраняет его в буфер по указателю buf, начиная с младшего байта
// Возвращает 1, если код прочитан, 0 - если импульс присутствия не получен
uint8_t onewire_read_rom(uint8_t * buf) {
  if (!onewire_reset())
    return 0; 
  onewire_send(0x33);
  for (uint8_t p = 0; p < 8; p++) {
    *(buf++) = onewire_read();
  }
  return 1;
}


// Выполняет последовательность инициализации (reset + ожидает импульс присутствия)
// Если импульс присутствия получен, выполняет команду MATCH ROM, и пересылает 8-байтовый код 
//   по указателю data (младший байт вперёд)
// Возвращает 1, если импульс присутствия получен, 0 - если нет
uint8_t onewire_match(uint8_t * data) {
  if (!onewire_reset())
    return 0;
  onewire_send(0x55);
  for (uint8_t p = 0; p < 8; p++) {
    onewire_send(*(data++));
  }
  return 1;
}


// Переменные для хранения промежуточного результата поиска
uint8_t onewire_enum[8]; // найденный восьмибайтовый адрес 
uint8_t onewire_enum_fork_bit; // последний нулевой бит, где была неоднозначность (нумеруя с единицы)

// Инициализирует процедуру поиска адресов устройств
void onewire_enum_init() {
  for (uint8_t p = 0; p < 8; p++) {
    onewire_enum[p] = 0;
  }      
  onewire_enum_fork_bit = 65; // правее правого
}

// Перечисляет устройства на шине 1-wire и получает очередной адрес.
// Возвращает указатель на буфер, содержащий восьмибайтовое значение адреса, либо NULL, если поиск завешён
uint8_t * onewire_enum_next() {
  if (!onewire_enum_fork_bit) { // Если на предыдущем шаге уже не было разногласий
    return 0; // то просто выходим ничего не возвращая
  }
  if (!onewire_reset()) {
    return 0;
  }  
  uint8_t bp = 8;
  uint8_t * pprev = &onewire_enum[0];
  uint8_t prev = *pprev;
  uint8_t next = 0;
  
  uint8_t p = 1;
  onewire_send(0xF0);
  uint8_t newfork = 0;
  for(;;) {
    uint8_t not0 = onewire_read_bit();
    uint8_t not1 = onewire_read_bit();
    if (!not0) { // Если присутствует в адресах бит ноль
      if (!not1) { // Но также присустствует бит 1 (вилка)
        if (p < onewire_enum_fork_bit) { // Если мы левее прошлого правого конфликтного бита, 
          if (prev & 1) {
            next |= 0x80; // то копируем значение бита из прошлого прохода
          } else {
            newfork = p; // если ноль, то запомним конфликтное место
          }          
        } else if (p == onewire_enum_fork_bit) {
          next |= 0x80; // если на этом месте в прошлый раз был правый конфликт с нулём, выведем 1
        } else {
          newfork = p; // правее - передаём ноль и запоминаем конфликтное место
        }        
      } // в противном случае идём, выбирая ноль в адресе
    } else {
      if (!not1) { // Присутствует единица
        next |= 0x80;
      } else { // Нет ни нулей ни единиц - ошибочная ситуация
        return 0;
      }
    }
    onewire_send_bit(next & 0x80);
    bp--;
    if (!bp) {
      *pprev = next;
      if (p >= 64)
        break;
      next = 0;
      pprev++;
      prev = *pprev;
      bp = 8;
    } else {
      if (p >= 64)
        break;
      prev >>= 1;
      next >>= 1;
    }
    p++;
  }
  onewire_enum_fork_bit = newfork;
  return &onewire_enum[0];
}

// Выполняет инициализацию (сброс) и, если импульс присутствия получен,
// выполняет MATCH_ROM для последнего найденного методом onewire_enum_next адреса
// Возвращает 0, если импульс присутствия не был получен, 1 - в ином случае
uint8_t onewire_match_last() {
  return onewire_match(&onewire_enum[0]);
}


// Отсылает по UART одну цифру, являющуюся результатом деления нацело dig на sub.
// Возвращает остаток этого деления
uint16_t uart_digit(uint16_t dig, uint16_t sub) {
  char c = '0';
  while (dig >= sub) {
    dig -= sub;
    c++;
  }
  
   One_wire_buf[one_w_i++]= c;
  return dig;
}



// Отсылает в UART десятичное представление числа с фиксированной точкой, 
// где дробная часть представлена младшими 4 разрядами
void uart_num(int16_t num) {
  uint16_t unum; // число без знака

    unum = num;
 
  uint16_t snum =  unum >> 4; // отбрасывает дробную часть
  if (snum >= 10) {
    if (snum >= 100) {
      if (snum >= 1000) {
		 
        snum = uart_digit(snum, 1000); // 4й разряд
      }
      snum = uart_digit(snum, 100); // 3й разряд
    }
    snum = uart_digit(snum, 10); // 2й разряд
  }
  uart_digit(snum, 1); // 1й разряд
  
 One_wire_buf[one_w_i++]= ','; // десятичный разделитель
  
  uart_digit((((uint8_t)(unum & 0x0F)) * 10) >> 4, 1); // дробная часть
 One_wire_buf[one_w_i++]= '*';
 One_wire_buf[one_w_i++]= 'C';
}


void send_skip_rom(void)
{
   if (onewire_skip())
		{onewire_send(0x44); // ...запускается замер температуры на всех термодатчиках
			do_1wire_or_nobody_home =1;
		}
	else {
One_wire_buf[0]='?';
		One_wire_buf[1]='?';
		One_wire_buf[2]='?';
		
	}	
}
