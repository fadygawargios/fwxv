#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "tasks.h"

#include "gpio.h"
#include "log.h"
#include "misc.h"
#include "delay.h"

// Non blocking delay. Simply consumes cpu cycles until a given time has passed
// static void prv_delay(const TickType_t delay_ms) {
//   TickType_t curr_tick = xTaskGetTickCount();
//   while(xTaskGetTickCount() - curr_tick < pdMS_TO_TICKS(delay_ms))
//   {}
// }

TASK(task1, TASK_STACK_512) {
  int counter1 = 0;
  while (true) {
    // Your code here
    LOG_DEBUG("Task 1\n");
    counter1++;
    delay_ms(100);
  }
}

TASK(task2, TASK_STACK_512) {
  int counter2 = 0;
  while (true) {
    // Your code here
    LOG_DEBUG("Task 2\n");
    counter2++;
    delay_ms(100);

  }
}

int main(void) {
    log_init();
    // Create tasks here
    tasks_init();
    tasks_init_task(task1, TASK_PRIORITY(1), NULL);
    tasks_init_task(task2, TASK_PRIORITY(1), NULL);


    tasks_start();
    LOG_DEBUG("Program start...\n");
    // Start the scheduler

    return 0;
}
