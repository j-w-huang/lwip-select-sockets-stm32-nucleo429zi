#include "main.h"

extern UART_HandleTypeDef huart3;

int __io_putchar(int ch) {
	HAL_UART_Transmit(&huart3, (uint8_t *) &ch, 1, 0xFFFF);
	return ch;
}


