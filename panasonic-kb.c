#include <fcntl.h>
#include <linux/uinput.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
// #include <time.h>
#include <unistd.h>

#define _KEY_DOWN(key) emit(io, EV_KEY, key, 1)
#define _KEY_UP(key) emit(io, EV_KEY, key, 0)
#define _REPORT() emit(io, EV_SYN, SYN_REPORT, 0)

#define KB_DEVICE "/dev/input/event0"
// Redefining Right Alt code for my system
#undef KEY_RIGHTALT
#define KEY_RIGHTALT 184

// Side keys
typedef enum
{
  APP1,
  APP2,
  APP3,
  APP4,
  UP,
  DOWN,
  ZOOM_IN,
  ZOOM_OUT,
} Buttons;

typedef struct
{
  uint32_t timestamp;  // Unix time
  uint32_t section_2;  // Dunno
  uint32_t section_3;  // Dunno 2
  uint32_t section_4;  // Button code in first event
} InputEvent;

typedef struct
{
  InputEvent* event[2];
  uint8_t events_count;
  int uinput;
  int keyboard;
} IO;

// For side keys as they don't report their status
uint8_t status[8] = {0, 0, 0, 0, 0, 0, 0, 0};

/* Copied from https://kernel.org/doc/html/v6.1/input/uinput.html and modified a little */
void emit(IO* io, int type, int code, int val)
{
  struct input_event event;

  event.type = type;
  event.code = code;
  event.value = val;
  event.time.tv_sec = 0;
  event.time.tv_usec = 0;

  write(io->uinput, &event, sizeof(event));
}

// Sending preread events
void send_event(IO* io, uint8_t event_number)
{
  write(io->uinput, io->event[event_number], 16);
}

/* Also copied from https://kernel.org/doc/html/v6.1/input/uinput.html and modified a bit more*/
int output_init()
{
  struct uinput_setup usetup;

  int uinput = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

  ioctl(uinput, UI_SET_EVBIT, EV_REL);
  ioctl(uinput, UI_SET_RELBIT, REL_WHEEL);
  ioctl(uinput, UI_SET_EVBIT, EV_KEY);

  // Setting 0-200 key codes because it's easier this way
  for (uint8_t i = 0; i < 200; i++)
  {
    ioctl(uinput, UI_SET_KEYBIT, i);
  }

  memset(&usetup, 0, sizeof(usetup));
  usetup.id.bustype = BUS_USB;
  usetup.id.vendor = 0xBABA;
  usetup.id.product = 0xDEDA;
  strcpy(usetup.name, "Virtual Input Device (silly)");

  ioctl(uinput, UI_DEV_SETUP, &usetup);
  ioctl(uinput, UI_DEV_CREATE);

  return uinput;
}

int input_init()
{
  // Opening keyboard in blocking mode and grabbing it
  int keyboard = open(KB_DEVICE, O_RDONLY);
  ioctl(keyboard, EVIOCGRAB, 1);
  return keyboard;
}

uint8_t io_init(IO* io)
{
  io->event[0] = malloc(sizeof(*io->event));
  io->event[1] = malloc(sizeof(*io->event));
  io->uinput = output_init();
  if (!io->uinput)
  {
    printf("Uinput initialization error\n\r");
    return 0;
  }
  io->keyboard = input_init();
  if (!io->keyboard)
  {
    printf("keyboard initialization error\n\r");
    return 0;
  }
  return 1;
}

void special_buttons(IO* io, uint32_t code)
{
  // printf("status: %u\n\r", status[code]);
  if (status[code] == 1)
  {
    status[code] = 0;
    return;
  }
  switch (code)
  {
    case APP1:
      // My terminal hotkey
      _KEY_DOWN(KEY_LEFTMETA);
      _KEY_DOWN(KEY_T);
      _REPORT();

      _KEY_UP(KEY_T);
      _KEY_UP(KEY_LEFTMETA);
      _REPORT();
      break;
    case APP2:
      // printf("APP2\n\r");
      break;
    case APP3:
      // printf("APP3\n\r");
      break;
    case APP4:
      // printf("APP4\n\r");
      break;
    case UP:
      // Scroll up
      emit(io, EV_REL, REL_WHEEL, 1);
      _REPORT();
      break;
    case DOWN:
      // Scroll down
      emit(io, EV_REL, REL_WHEEL, -1);
      _REPORT();
      break;
    case ZOOM_IN:
      _KEY_DOWN(KEY_LEFTALT);
      _KEY_DOWN(KEY_F9);
      _REPORT();

      _KEY_UP(KEY_F9);
      _KEY_UP(KEY_LEFTALT);
      _REPORT();
      break;
    case ZOOM_OUT:
      // printf("Zoom-\n\r");
      break;
    default:
      printf("Something's wrong\n\r");
      break;
  }
  status[code] = 1;
}

void sticky_keys(uint8_t code)
{
  // TODO
  switch (code)
  {
    case KEY_LEFTALT:
      printf("Left alt\n\r");
      break;
    case KEY_RIGHTALT:
      printf("Right alt\n\r");
      break;
    case KEY_LEFTCTRL:
      printf("Left ctrl\n\r");
      break;
    case KEY_LEFTSHIFT:
      printf("Left shift\n\r");
      break;
    default:
      // What
      break;
  }
}

void get_input(IO* io)
{
  io->events_count = 0;

  read(io->keyboard, io->event[io->events_count], 16);
  // printf("%u, %x, %x, %x\n\r", io->event[0]->timestamp,
  //        io->event[0]->section_2, io->event[0]->section_3,
  //        io->event[0]->section_4);

  if (io->event[0]->section_3 != 0x00040004) return;
  io->events_count = 1;

  read(io->keyboard, io->event[io->events_count], 16);

  if ((io->event[1]->section_3 | io->event[1]->section_4) == 0)
  {
    // printf("Syn_report!\n\r");
    if (io->event[0]->section_4 < 108 || io->event[0]->section_4 > 100)
    {
      printf("Special key\n\r");
      special_buttons(io, io->event[0]->section_4 - 101);
    }
    return;
  }
  io->events_count = 2;

  InputEvent event;
  memset(&event, 0, 16);

  read(io->keyboard, &event, 16);

  if ((event.section_3 | event.section_4) != 0)
    printf("Something's wrong!\n\r");
}

int main()
{
  printf("It's on!\n\r");

  IO* io = malloc(sizeof(*io));
  if (!io)
  {
    printf("Memory allocation error!\n\r");
    return 1;
  }

  if (!io_init(io))
  {
    printf("IO initialization error\n\r");
    return 1;
  }

  uint8_t running = 1;
  while (running)
  {
    get_input(io);

    if (io->events_count > 2 || io->events_count == 0)
    {
      printf("%u events, skipping\n\r", io->events_count);
      continue;
    }
    for (uint8_t i = 0; i < io->events_count; i++)
    {

      send_event(io, i);
    }
    _REPORT();
  }

  close(io->uinput);
  close(io->keyboard);
  free(io->event[0]);
  free(io->event[1]);
  free(io);
  return 0;
}
