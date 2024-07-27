#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "tasks.h"
#include "queues.h"
#include "status.h"
#include "delay.h"
#include "log.h"
#include "misc.h"

#define ITEM_SZ 6
#define QUEUE_LEN 5
#define BUF_SIZE (QUEUE_LEN * ITEM_SZ)

static const char s_list[QUEUE_LEN][ITEM_SZ] = {
    "Item1",
    "Item2",
    "Item3",
    "Item4",
    "Item5"
};

// Task static entities
static uint8_t s_queue1_buf[BUF_SIZE];
static Queue s_queue1 = {
    .num_items = QUEUE_LEN,
    .item_size = ITEM_SZ,
    .storage_buf = s_queue1_buf,
};

TASK(task1, TASK_STACK_512) {
    LOG_DEBUG("Task 1 initialized!\n");
    StatusCode ret;
    int to_send = 0;
    while (true) {
        ret = queue_send(&s_queue1, &s_list[to_send], 0);
        delay_ms(100);
        if (ret != STATUS_CODE_OK) {
            LOG_DEBUG("Write to queue failed!");
        }
        to_send++;
        if (to_send >= QUEUE_LEN) {
            break;
        }
    }
}

TASK(task2, TASK_STACK_512) {
    LOG_DEBUG("Task 2 initialized!\n");
    char outstr[ITEM_SZ];
    StatusCode ret;
    while (true) {
        ret = queue_receive(&s_queue1, &outstr, 1000);
        delay_ms(500);
        if (ret == STATUS_CODE_OK) {
            LOG_DEBUG("%s", outstr);
        } else {
            LOG_DEBUG("Read from queue failed");
            break;
        }
    }
}

int main(void) {
    log_init();
    tasks_init();
    
    // Initialize queues here
    queue_init(&s_queue1);
    
    tasks_init_task(task1, TASK_PRIORITY(2), NULL);
    tasks_init_task(task2, TASK_PRIORITY(2), NULL); 
    LOG_DEBUG("Program start...\n");
    tasks_start();
    
    return 0;
}