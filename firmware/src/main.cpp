#include <cstring>
#include "stdio.h"

#include "CANFD.hpp"
#include "FullColorLED.hpp"
#include "WS2812B.hpp"

#include "messageFormat/powerboard.hpp"

extern DMA_HandleTypeDef hdma_tim2_ch1;

CANFD *canfd;
FullColorLED led{&htim1, TIM_CHANNEL_1};
WS2812B status_LED{&htim2, TIM_CHANNEL_1, &hdma_tim2_ch1};
int data;
extern osTimerId_t dischargeTimerHandle;

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim)
{
  if (htim == &htim2)
  {
    status_LED.do_forwardRewrite();
  }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  if (htim == &htim2)
  {
    status_LED.do_backRewrite();
  }
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
  {
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
      led.set_rgb(0, 255, 0);
    }
    else
    {
      CANFD_Frame emengency_msg;
      ID_Format id;
      id.format.broadcast = true;
      id.format.from_BoardID = 0;
      id.format.from_BoardType = Board_Type::PowerBoard;

      emengency_msg.id = id.id;
      emengency_msg.is_remote = true;
      emengency_msg.size = 0;
      canfd->tx(emengency_msg);
      HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_SET);
      osTimerStart(dischargeTimerHandle, 300);
    }
  }
}

void Relay_ONOFF(bool ONOFF)
{
  if (ONOFF == 1)
  {
    HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_RESET);
    osDelay(5);
    if (HAL_GPIO_ReadPin(EMENGECY_GPIO_Port, EMENGECY_Pin))
    {
      led.set_rgb(255, 255, 0);
    }
    else
    {
      led.set_rgb(255, 255, 0);
    }
    HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_SET);
  }
  else
  {
    led.set_rgb(255, 0, 0);
    HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_RESET);
    osDelay(30);
    HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_SET);
    osTimerStart(dischargeTimerHandle, 300);
  }
}

extern "C" void StartDefaultTask(void *argument)
{

  led.set_rgb(255, 0, 0);
  led.start();
  canfd = new CANFD(&hfdcan1);
  canfd->start();

  ID_Format filter_id;
  filter_id.format.from_BoardType = Board_Type::Master_Board;
  filter_id.format.to_BoardType = Board_Type::PowerBoard;
  filter_id.format.message_type = Message_Type::Target;
  filter_id.format.to_BoardID = 0;
  canfd->set_filter_mask(filter_id.id, 0x7ff);

  CANFD_Frame test;
  test.id = 10;
  test.size = 32;
  memset(test.data, 0, 64);
  canfd->tx(test);

  float ad;
  while (1)
  {
    if (canfd->rx_available())
    {
      CANFD_Frame data;
      canfd->rx(data);
      auto msg = reinterpret_cast<PowerBoard_Target*>(&data.data);
      Relay_ONOFF(msg->ON_OFF);
      HAL_ADC_Start(&hadc1);
      if (HAL_ADC_PollForConversion(&hadc1, 1000) == HAL_OK)
      {
        ad = (HAL_ADC_GetValue(&hadc1) - 350) / 62.0;
      }
      HAL_ADC_Stop(&hadc1);

      osDelay(10);
    }
  }
}

extern "C" void controllLEDTask(void *argument)
{
  for (int i = 0; i < 50; i++)
  {
    status_LED.set_rgb(i, 255, 255, 255);
  }
  status_LED.show();
  while (1)
  {
    osDelay(100);
  }
}

extern "C" void dischargeCallback(void *argument)
{
  HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_RESET);
}