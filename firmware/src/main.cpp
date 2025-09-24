#include "main.h"
#include <cstring>
#include "stdio.h"

#include "message.hpp"
#include "CANFD.hpp"
#include "FullColorLED.hpp"

CANFD* canfd;
FullColorLED led{&htim1, TIM_CHANNEL_1};
int data;
extern osTimerId_t dischargeTimerHandle;

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) {
	  canfd->rx_interrupt_task();
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == EMENGECY_Pin)
  {
    if (HAL_GPIO_ReadPin(EMENGECY_GPIO_Port, EMENGECY_Pin) == GPIO_PIN_RESET)
    {
      HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_RESET);
    }
    else
    {
      HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_SET);
    }
  }
}

extern "C" void StartDefaultTask(void *argument)
{

  led.set_rgb(255, 0, 0);
  led.start();
  canfd = new CANFD(&hfdcan1);
	canfd->start();
  canfd->set_filter_mask(1048576, 0xFFC000);
  

	CANFD_Frame test;
	test.id=10;
	test.size = 32;
	memset(test.data, 0, 64);

	//canfd->tx(test);

  while (1)
  {
    if (canfd->rx_available())
    {
      CANFD_Frame data;
      canfd->rx(data);
      Message_format msg = {0};
      memcpy(&msg.data, data.data, 32);
      if (msg.data.power_rsv.ON_OFF == 1){
		    led.set_rgb(0, 255, 0);
        HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_RESET);
        osDelay(10);
        HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_SET);
	    } else {
		    led.set_rgb(255, 0, 0);
        HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_RESET);
        osDelay(30);
        HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_SET);
        osTimerStart(dischargeTimerHandle, 300);
	    }
    }
    osDelay(10);
  }
}

extern "C" void dischargeCallback(void *argument)
{
    HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_RESET);
}