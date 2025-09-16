#include "main.h"
#include <cstring>
#include "stdio.h"

#include "message.hpp"
#include "CANFD.hpp"
#include "FullColorLED.hpp"

CANFD* canfd;
FullColorLED led{&htim1, TIM_CHANNEL_1};
int data;

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) {
	  canfd->rx_interrupt_task();
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
        
	    } else {
		    led.set_rgb(255, 0, 0);
	    }

    }
    osDelay(10);
  }
}