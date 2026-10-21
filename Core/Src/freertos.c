/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "kb.h"
#include "oled.h"
#include "buzzer.h"
#include "tetris.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// в очередь eventQueue пишут два источника: задача клавиатуры (код кнопки)
// и программный таймер (событие падения), чтобы их различать падение имеет код больше 0xFF
#define EVT_FALL  0x100

#define KB_PERIOD     20   // период опроса клавиатуры, мс
#define REPEAT_START  300  // через сколько мс удержания кнопка начинает повторяться
#define REPEAT_PERIOD 100  // период повтора, мс
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId InputTaskHandle;
osThreadId GameTaskHandle;
osThreadId DrawTaskHandle;
osMessageQId eventQueueHandle;
osTimerId fallTimerHandle;
osTimerId beepTimerHandle;
osMutexId i2cMutexHandle;
osMutexId gameMutexHandle;
osSemaphoreId drawSemHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void Beep(uint32_t freq, uint32_t ms);
static void UpdateLeds(void);
/* USER CODE END FunctionPrototypes */

void StartInputTask(void const * argument);
void StartGameTask(void const * argument);
void StartDrawTask(void const * argument);
void FallTimerCallback(void const * argument);
void BeepTimerCallback(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* GetTimerTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* USER CODE BEGIN GET_TIMER_TASK_MEMORY */
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
  /* place for user code */
}
/* USER CODE END GET_TIMER_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of i2cMutex */
  osMutexDef(i2cMutex);
  i2cMutexHandle = osMutexCreate(osMutex(i2cMutex));

  /* definition and creation of gameMutex */
  osMutexDef(gameMutex);
  gameMutexHandle = osMutexCreate(osMutex(gameMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of drawSem */
  osSemaphoreDef(drawSem);
  drawSemHandle = osSemaphoreCreate(osSemaphore(drawSem), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* definition and creation of fallTimer */
  osTimerDef(fallTimer, FallTimerCallback);
  fallTimerHandle = osTimerCreate(osTimer(fallTimer), osTimerPeriodic, NULL);

  /* definition and creation of beepTimer */
  osTimerDef(beepTimer, BeepTimerCallback);
  beepTimerHandle = osTimerCreate(osTimer(beepTimer), osTimerOnce, NULL);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of eventQueue */
  osMessageQDef(eventQueue, 16, uint16_t);
  eventQueueHandle = osMessageCreate(osMessageQ(eventQueue), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of InputTask */
  osThreadDef(InputTask, StartInputTask, osPriorityAboveNormal, 0, 256);
  InputTaskHandle = osThreadCreate(osThread(InputTask), NULL);

  /* definition and creation of GameTask */
  osThreadDef(GameTask, StartGameTask, osPriorityNormal, 0, 512);
  GameTaskHandle = osThreadCreate(osThread(GameTask), NULL);

  /* definition and creation of DrawTask */
  osThreadDef(DrawTask, StartDrawTask, osPriorityBelowNormal, 0, 384);
  DrawTaskHandle = osThreadCreate(osThread(DrawTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartInputTask */
/**
  * @brief  Function implementing the InputTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartInputTask */
void StartInputTask(void const * argument)
{
  /* USER CODE BEGIN StartInputTask */
  char key;
  char lastKey = 0;
  uint32_t holdTime = 0;

  /* Infinite loop */
  for(;;)
  {
    // шина I2C общая с дисплеем
    osMutexWait(i2cMutexHandle, osWaitForever);
    key = KB_Scan();
    osMutexRelease(i2cMutexHandle);

    if (key != 0 && key != lastKey) {
      // новое нажатие
      osMessagePut(eventQueueHandle, key, 0);
      holdTime = 0;
    } else if (key != 0 && (key == '4' || key == '6' || key == '8')) {
      // кнопки движения при удержании повторяются
      holdTime += KB_PERIOD;
      if (holdTime >= REPEAT_START) {
        osMessagePut(eventQueueHandle, key, 0);
        holdTime = REPEAT_START - REPEAT_PERIOD;
      }
    }
    lastKey = key;

    osDelay(KB_PERIOD);
  }
  /* USER CODE END StartInputTask */
}

/* USER CODE BEGIN Header_StartGameTask */
/**
* @brief Function implementing the GameTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartGameTask */
void StartGameTask(void const * argument)
{
  /* USER CODE BEGIN StartGameTask */
  osEvent event;
  uint8_t res;
  uint32_t speed;
  uint32_t oldSpeed;

  Tetris_Init();
  UpdateLeds();
  osSemaphoreRelease(drawSemHandle);

  // таймер падения работает всегда, в меню и на паузе событие просто игнорируется
  oldSpeed = Tetris_GetSpeed();
  osTimerStart(fallTimerHandle, oldSpeed);

  /* Infinite loop */
  for(;;)
  {
    event = osMessageGet(eventQueueHandle, osWaitForever);
    if (event.status != osEventMessage) {
      continue;
    }

    // состояние игры читает еще задача отрисовки, поэтому меняем его под мьютексом
    osMutexWait(gameMutexHandle, osWaitForever);
    if (event.value.v == EVT_FALL) {
      res = Tetris_Fall();
    } else {
      res = Tetris_Key((char)event.value.v);
    }
    speed = Tetris_GetSpeed();
    osMutexRelease(gameMutexHandle);

    if (res == RES_NONE) {
      continue;
    }

    if (res == RES_LOCK) Beep(400, 30);
    if (res == RES_LINES) Beep(1500, 150);
    if (res == RES_OVER) Beep(200, 700);

    // с новым уровнем фигуры падают быстрее
    if (speed != oldSpeed) {
      osTimerStart(fallTimerHandle, speed);
      oldSpeed = speed;
    }

    UpdateLeds();
    osSemaphoreRelease(drawSemHandle);
  }
  /* USER CODE END StartGameTask */
}

/* USER CODE BEGIN Header_StartDrawTask */
/**
* @brief Function implementing the DrawTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDrawTask */
void StartDrawTask(void const * argument)
{
  /* USER CODE BEGIN StartDrawTask */
  /* Infinite loop */
  for(;;)
  {
    // ждем сигнала от игры что надо перерисовать экран
    osSemaphoreWait(drawSemHandle, osWaitForever);

    // буфер дисплея трогает только эта задача, а состояние игры защищено мьютексом
    osMutexWait(gameMutexHandle, osWaitForever);
    Tetris_Draw();
    osMutexRelease(gameMutexHandle);

    // отправка буфера на дисплей долгая (~100 мс), в это время игра может работать дальше
    osMutexWait(i2cMutexHandle, osWaitForever);
    oled_UpdateScreen();
    osMutexRelease(i2cMutexHandle);
  }
  /* USER CODE END StartDrawTask */
}

/* FallTimerCallback function */
void FallTimerCallback(void const * argument)
{
  /* USER CODE BEGIN FallTimerCallback */
  // в функции таймера нельзя блокироваться, поэтому таймаут 0
  osMessagePut(eventQueueHandle, EVT_FALL, 0);
  /* USER CODE END FallTimerCallback */
}

/* BeepTimerCallback function */
void BeepTimerCallback(void const * argument)
{
  /* USER CODE BEGIN BeepTimerCallback */
  Buzzer_Off();
  /* USER CODE END BeepTimerCallback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

// включаем звук, а выключит его однократный программный таймер beepTimer
static void Beep(uint32_t freq, uint32_t ms)
{
  Buzzer_On(freq);
  osTimerStart(beepTimerHandle, ms);
}

// зеленый - игра идет, желтый - пауза, красный - конец игры
static void UpdateLeds(void)
{
  uint8_t st = Tetris_GetState();

  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, st == GAME_RUN ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin, st == GAME_PAUSE ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, st == GAME_OVER ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* USER CODE END Application */
