#include "main.h"
#include <cstring>
#include "stdio.h"

#include "CANFD.hpp"
#include "FullColorLED.hpp"

CANFD* canfd;
FullColorLED led{&htim1, TIM_CHANNEL_1};

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) {
	  canfd->rx_interrupt_task();
    led.set_rgb(255, 0, 0);
  }
}

extern "C" void StartDefaultTask(void *argument)
{

  led.set_rgb(255, 0, 0);
  led.start();
  printf("start\n");
  canfd = new CANFD(&hfdcan1);
	canfd->init();

	CANFD_Frame test;
	test.id=10;
	test.size = 32;
	memset(test.data, 0, 64);
	//canfd->tx(test);

  while (1)
  {
    led.set_rgb(0, 0, 0);
    HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_SET);
    osDelay(10000);
    HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_RESET);
    osDelay(100);
    HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_SET);
    osDelay(1000);
    HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_RESET);
    led.set_rgb(0, 0, 0);
    osDelay(10000);
  }
}