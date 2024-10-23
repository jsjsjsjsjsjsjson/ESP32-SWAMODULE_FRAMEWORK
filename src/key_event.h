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

#define KEY_UP 1
#define KEY_DOWN 2
#define KEY_L 3
#define KEY_R 4
#define KEY_OK 5
#define KEY_S 7
#define KEY_P 8
#define KEY_MENU 9
#define KEY_NAVI 10
#define KEY_BACK 11
#define KEY_OCTU 12
#define KEY_OCTD 13

#endif