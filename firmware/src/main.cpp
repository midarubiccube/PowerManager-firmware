#include <stdio.h>
#include "main.h"
#include "FullColorLED.hpp"
#include "cmsis_os.h"
#include "FreeRTOS.h"

extern "C" void StartDefaultTask(void *argument)
{
  FullColorLED led{&htim1, TIM_CHANNEL_1};
  led.set_rgb(255, 0, 0);
  led.start();

  while (1)
  {
    led.set_rgb(0, 0, 0);
    /*HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_SET);
    osDelay(10000);
    HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_RESET);
    osDelay(100);*/
    HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_SET);
    /*osDelay(1000);
    HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_RESET);*/
    led.set_rgb(0, 0, 0);
    osDelay(10000);
  }
}