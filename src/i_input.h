#ifndef __I_INPUT_H
#define __I_INPUT_H

//extern void *n64_malloc(size_t size_to_alloc);
/*
const char *ACTION_SHIFT = "SHIFT";
const char *ACTION_SHOOT = "SHOOT";
const char *ACTION_USE = "USE";
const char *ACTION_LSTRAFE = "LSTRAFE";
const char *ACTION_RSTRAFE = "RSTRAFE";
const char *ACTION_AUTOMAP = "AUTOMAP";
const char *ACTION_ENTER = "ENTER";
const char *ACTION_WEAPON_DOWN = "WEAPON_DOWN";
const char *ACTION_WEAPON_UP = "WEAPON_UP";
const char *ACTION_UP = "UP";
const char *ACTION_DOWN = "DOWN";
const char *ACTION_LEFT = "LEFT";
const char *ACTION_RIGHT = "RIGHT";
const char *ACTION_ESC = "ESC";

typedef enum action_e
{
	ca_shift,
	ca_shoot,
	ca_use,
	ca_lstrafe,
	ca_rstrafe,
	ca_automap,
	ca_enter,
	ca_weapon_down,
	ca_weapon_up,
	ca_up,
	ca_down,
	ca_left,
	ca_right,
	ca_esc
} control_action;

typedef enum button_e
{
    cb_z,
	cb_a,
	cb_b,
	cb_lt,
	cb_rt,
	cb_cu,
	cb_cd,
	cb_cl,
	cb_cr,
	cb_u,
	cb_d,
	cb_l,
	cb_r,
	cb_start
} control_button;

const char *BUTTON_Z = "Z";
const char *BUTTON_A = "A";
const char *BUTTON_B = "B";
const char *BUTTON_LT = "LT";
const char *BUTTON_RT = "RT";
const char *BUTTON_CU = "CU";
const char *BUTTON_CD = "CD";
const char *BUTTON_CL = "CL";
const char *BUTTON_CR = "CR";
const char *BUTTON_U = "U";
const char *BUTTON_D = "D";
const char *BUTTON_L = "L";
const char *BUTTON_R = "R";
const char *BUTTON_START = "START";

typedef struct control_map_s
{
	control_action action;
	control_button button;
} control_map_t;


control_map_t control_map[14];
*/
/*typedef enum button
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

void translate_n64_control(button_t button, button_state_t state);


typedef struct input_event_s
{

	button_t	button;
	button_state_t	state;

} input_event_t;

typedef struct queue_s
{

	input_event_t	entry[256];
	int		size;

} queue_t;

void I_GetEvent(void);


void queue_init(queue_t *queue)
{
	queue->size = 0;

	for(int i=0;i<256;i++)
	{
		input_event_t event;
		event.button = BUTTON_END;
		event.state = BUTTON_STATE_END;

		queue->entry[i] = event;
	}
}


void queue_push(input_event_t *entry, queue_t *queue)
{
	if(queue->size == 255)
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
}*/


/**
 * MUST CALL QUEUE_SIZE FIRST, IF 0, DO NOT POP!
 */
/*input_event_t *queue_pop(queue_t *queue)
{
	input_event_t *entry = (input_event_t*)n64_malloc(1 * sizeof(input_event_t));

	entry->button = queue->entry[0].button;
	entry->state = queue->entry[0].state;

	for(int i=0;i<queue->size-1;i++)
	{
		queue->entry[i].button = queue->entry[i+1].button;
		queue->entry[i].state = queue->entry[i+1].state;
	}

	queue->size -= 1;

	return entry;
}*/

#endif // __I_INPUT_H
