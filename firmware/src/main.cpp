#include <cstring>
#include "stdio.h"
#include "math.h"

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
extern osTimerId_t WatchDogTaskHandle;

bool ONOFF_flg = false;
bool emengecy_flag = false;
uint32_t last_receive = 0;
uint8_t rgb;

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

  if (GPIO_Pin == EMENGECY_Pin && ONOFF_flg)
  {
    CANFD_Frame emengency_msg;
    ID_Format id;
    id.format.broadcast = true;
    id.format.from_BoardID = 0;
    id.format.from_BoardType = Board_Type::PowerBoard;
    id.format.message_type = Message_Type::EMENGECY;
    if (HAL_GPIO_ReadPin(EMENGECY_GPIO_Port, EMENGECY_Pin) == GPIO_PIN_RESET)
    {
      HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_RESET);
      led.set_rgb(0, 255, 0);
      rgb = 0;
      emengency_msg.data[0] = 0;
    }
    else
    {
      HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_SET);
      led.set_rgb(100, 50, 0);
      rgb = 255;
      emengency_msg.data[0] = 1;
      osTimerStart(dischargeTimerHandle, 300);
    }
    emengency_msg.id = id.id;
    emengency_msg.size = 5;
    canfd->tx(emengency_msg);
  }
}

void Relay_ONOFF(bool ONOFF)
{
  if (ONOFF && !emengecy_flag)
  {
    HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_RESET);
    osDelay(5);
    if (HAL_GPIO_ReadPin(EMENGECY_GPIO_Port, EMENGECY_Pin))
    {
      led.set_rgb(100, 50, 0);
    }
    else
    {
      led.set_rgb(0, 255, 0);
      rgb = 128;
    }
    HAL_GPIO_WritePin(ONOFF_GPIO_Port, ONOFF_Pin, GPIO_PIN_SET);
    ONOFF_flg = true;
  }
  else
  {
    ONOFF_flg = false;
    led.set_rgb(255, 0, 0);
    rgb = 255;
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
  filter_id.format.broadcast = true;
  canfd->set_filter_mask(0, filter_id.id, filter_id.id);

  filter_id.id = 0;
  filter_id.format.to_BoardType = Board_Type::PowerBoard;
  filter_id.format.to_BoardID = 0;
  canfd->set_filter_mask(1, filter_id.id, 0xFF);

  CANFD_Frame test;
  test.id = 10;
  test.size = 32;
  memset(test.data, 0, 64);
  canfd->tx(test);

  float ad;
  osTimerStart(WatchDogTaskHandle, 500);

  while (1)
  {
    if (canfd->rx_available())
    {
      CANFD_Frame data;
      canfd->rx(data);

      ID_Format rsv_id;
      rsv_id.id = data.id;
      if (rsv_id.format.from_BoardType == Board_Type::Master_Board && rsv_id.format.message_type == Message_Type::Target)
      {
        auto target = reinterpret_cast<PowerBoard_Target *>(&data.data);
        Relay_ONOFF(target->ON_OFF);
      }
      else if (rsv_id.format.broadcast == true && rsv_id.format.message_type == Message_Type::EMENGECY)
      {
        if (data.data[0] == 1)
        {
          emengecy_flag = true;
          Relay_ONOFF(false);
        }
        else
        {
          emengecy_flag = false;
          Relay_ONOFF(true);
        }
      }
      last_receive = HAL_GetTick();
    }

    osDelay(10);
  }
}

extern "C" void statusTaskFunc(void *argument)
{
  osDelay(1000);
  for (;;)
  {
    float ad;
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 1000) == HAL_OK)
    {
      ad = ((float)HAL_ADC_GetValue(&hadc1) - 350.0) / 62.0;
    }
    HAL_ADC_Stop(&hadc1);
    PowerBoard_Status status = {0};
    ID_Format id;
    id.format.from_BoardID = 0;
    id.format.from_BoardType = Board_Type::PowerBoard;
    id.format.to_BoardID = 0;
    id.format.to_BoardType = Board_Type::Master_Board;
    id.format.message_type = Message_Type::Status;

    status.Current = ad;
    CANFD_Frame sendmsg;
    sendmsg.id = id.id;
    sendmsg.size = sizeof(PowerBoard_Status);
    memcpy(sendmsg.data, &status, sizeof(PowerBoard_Status));
    // canfd->tx(sendmsg);
    osDelay(100);
  }
}

void rainbow(int count, uint8_t *rgb)
{
  int x = count / 255;
  int y = count % 255;

  if (x == 0)
    rgb[0] = 255, rgb[1] = y, rgb[2] = 0;
  else if (x == 1)
    rgb[0] = 255 - y, rgb[1] = 255, rgb[2] = 0;
  else if (x == 2)
    rgb[0] = 0, rgb[1] = 255, rgb[2] = y;
  else if (x == 3)
    rgb[0] = 0, rgb[1] = 255 - y, rgb[2] = 255;
  else if (x == 4)
    rgb[0] = y, rgb[1] = 0, rgb[2] = 255;
  else if (x == 5)
    rgb[0] = 255, rgb[1] = 0, rgb[2] = 255 - y;
}


int count = 0;

extern "C" void controllLEDTask(void *argument)
{
  for (int i = 0; i < 64; i++)
  {
    status_LED.set_rgb(i, 0, 0, 0);
  }
  status_LED.show();
  while (1)
  {
    for (int i = 0; i < 110; i++)
    {
      count += 255;
      if (count > 1530)
        count = 0;
      uint8_t rgb[3];
      rainbow(count, rgb);
      status_LED.set_rgb(i, rgb[0], rgb[1], rgb[2]);
    }
    status_LED.show();
    osDelay(100);
  }
}

extern "C" void WatchDogCallback(void *argument)
{
  if (HAL_GetTick() - last_receive > 2000)
  {
    Relay_ONOFF(false);
  }
}

extern "C" void dischargeCallback(void *argument)
{
  HAL_GPIO_WritePin(DISCHARGE_GPIO_Port, DISCHARGE_Pin, GPIO_PIN_RESET);
}