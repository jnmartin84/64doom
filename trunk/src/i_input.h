#ifndef __I_INPUT_H
#define __I_INPUT_H

extern void *n64_malloc(size_t size_to_alloc);

typedef enum button
{
    BUTTON_A,
    BUTTON_B,
    BUTTON_START,
    BUTTON_DPAD_UP,
    BUTTON_DPAD_DOWN,
    BUTTON_DPAD_LEFT,
    BUTTON_DPAD_RIGHT,
    BUTTON_L,
    BUTTON_R,
    BUTTON_C_UP,
    BUTTON_C_DOWN,
    BUTTON_C_LEFT,
    BUTTON_C_RIGHT,
    BUTTON_Z,
    BUTTON_ANALOG_UP,
    BUTTON_ANALOG_DOWN,
    BUTTON_ANALOG_LEFT,
    BUTTON_ANALOG_RIGHT,
    BUTTON_END
} button_t;


typedef enum button_state
{
    BUTTON_PRESSED,
    BUTTON_HELD,
    BUTTON_RELEASED,
    BUTTON_STATE_END
} button_state_t;


typedef struct input_event_s
{
    button_t       button;
    button_state_t state;
} input_event_t;


typedef struct queue_s
{
    input_event_t entry[256];
    int           size;
} queue_t;


void translate_n64_control(button_t button, button_state_t state);
void I_GetEvent(void);


void queue_init(queue_t *queue)
{
    int i;

    queue->size = 0;

    for (i=0;i<256;i++)
    {
        input_event_t event;
        event.button = BUTTON_END;
        event.state = BUTTON_STATE_END;

        queue->entry[i] = event;
    }
}


void queue_push(input_event_t *entry, queue_t *queue)
{
    if (queue->size == 255)
    {
        return;
    }
    else
    {
        queue->entry[queue->size].button = entry->button;
        queue->entry[queue->size].state = entry->state;
        queue->size += 1;
    }
}


int queue_size(queue_t *queue)
{
    return queue->size;
}


/**
 * MUST CALL QUEUE_SIZE FIRST, IF 0, DO NOT POP!
 */
input_event_t *queue_pop(queue_t *queue)
{
    int i;
    input_event_t *entry = (input_event_t*)n64_malloc(1 * sizeof(input_event_t));

    entry->button = queue->entry[0].button;
    entry->state = queue->entry[0].state;

    for (i=0;i<queue->size-1;i++)
    {
        queue->entry[i].button = queue->entry[i+1].button;
        queue->entry[i].state = queue->entry[i+1].state;
    }

    queue->size -= 1;

    return entry;
}

#endif // __I_INPUT_H
