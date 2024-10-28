#ifndef KEY_EVENT_H
#define KEY_EVENT_H

#include <freertos/queue.h>
#include <stdint.h>

typedef enum {
    KEY_IDLE,
    KEY_ATTACK,
    KEY_RELEASE
} key_status_t;

typedef struct {
    uint8_t num;
    key_status_t status;
} key_event_t;

QueueHandle_t xOptionKeyQueue;
QueueHandle_t xTouchPadQueue;
QueueHandle_t xNoteQueue;
#define readOptionKeyEvent xQueueReceive(xOptionKeyQueue, &optionKeyEvent, 0)
#define readTouchPadKeyEvent xQueueReceive(xTouchPadQueue, &touchPadEvent, 0)
#define readNoteEvent xQueueReceive(xNoteQueue, &noteEvent, 0)

#endif