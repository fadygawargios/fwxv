#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "tasks.h"

#include "gpio.h"
#include "log.h"
#include "misc.h"
#include "delay.h"

// Non blocking delay. Simply consumes cpu cycles until a given time has passed
static void prv_delay(const TickType_t delay_ms) {
  vTaskDelay(pdMS_TO_TICKS(delay_ms));
}

TASK(task1, TASK_STACK_512) {
  int counter1 = 0;
  while (true) {
    LOG_DEBUG("Task1 %d\n", counter1);
    counter1++;
    prv_delay(1000);
  }
}

TASK(task2, TASK_STACK_512) {
  int counter2 = 0;
  while (true) {
    LOG_DEBUG("Task2 %d\n", counter2);
    counter2++;
    prv_delay(1000);
  }
}

int main(void) {
    log_init();
    tasks_init();

    LOG_DEBUG("Program start...\n");
    
    tasks_init_task(task1, TASK_PRIORITY(2), NULL);
    tasks_init_task(task2, TASK_PRIORITY(3), NULL);

    tasks_start();

    return 0;
}