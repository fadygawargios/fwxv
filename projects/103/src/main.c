#include <stdio.h>

#include "gpio.h"
#include "log.h"
#include "master_task.h"
#include "tasks.h"

void pre_loop_init() {}

void run_fast_cycle() {}

void run_medium_cycle() {}

void run_slow_cycle() {}

int main() {
  gpio_init();
  tasks_init();
  log_init();
  LOG_DEBUG("Welcome to TEST!");

  init_master_task();

  tasks_start();

  LOG_DEBUG("exiting main?");
  return 0;
}
