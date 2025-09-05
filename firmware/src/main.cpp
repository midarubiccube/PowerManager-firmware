#include "main.h"
#include <cstring>
#include "stdio.h"

#include "CANFD.hpp"
#include "FullColorLED.hpp"

CANFD* canfd;
FullColorLED led{&htim1, TIM_CHANNEL_1};
int data;

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
  canfd = new CANFD(&hfdcan1);
	canfd->start();
  

	CANFD_Frame test;
	test.id=10;
	test.size = 32;
	memset(test.data, 0, 64);

	//canfd->tx(test);

  while (1)
  {
    printf("start\n");
    led.set_rgb(0, 0, 0);
    osDelay(10000);
  }
}