#include "main.h"
#include "adc.h"
#include "dma.h"
#include "fdcan.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "FullColorLED.hpp"
#include "spi.h"
#include <stdio.h>

extern "C" void SystemClock_Config(void);

int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart1,(uint8_t *)ptr,len,10);
  return len;
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  setbuf(stdout, NULL);

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_FDCAN1_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();

  FullColorLED led{&htim1, TIM_CHANNEL_1};
  led.set_rgb(255, 0, 0);
  led.start();

  while (1)
  {
    led.set_rgb(255, 255, 255);
    HAL_Delay(100);
    led.set_rgb(0, 0, 255);;    
    HAL_Delay(100);
    led.set_rgb(255, 0, 0);;    
    HAL_Delay(100);
    led.set_rgb(0, 255, 0);;    
    HAL_Delay(100);
    led.set_rgb(255, 255, 0);;    
    HAL_Delay(100);
    led.set_rgb(255, 0, 255);;    
    HAL_Delay(100);
    led.set_rgb(0, 0, 255);;    
    HAL_Delay(100);
    uint16_t adcv[3] = {0};
    for (int i = 0; i < 3; i++) {
      adcv[i] = MCP3208_read_u16(i);
    }
    /*HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_RESET);
    HAL_Delay(1000);*/
    printf("Read VAL:\t%d\t%d\t%d\r\n", adcv[0], adcv[1], adcv[2]);
  }
  /* USER CODE END 3 */
}