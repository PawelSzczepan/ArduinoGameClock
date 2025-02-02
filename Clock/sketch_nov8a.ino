#include "max7219.h"

#define LEFT 0
#define RIGHT 1

#define BUTTON_START D3
#define BUTTON_UP D10
#define BUTTON_DOWN D4

#define BLINKTIME 500
#define BLINKSTOPTIME 100

MAX7219 max7219;

enum devicemode_t {
  COUNTDOWN,
  TIMEOUT,
  STOP
};

devicemode_t mode = COUNTDOWN;

float countdown_reset_time = 30.000f;
float current_cd_time_left = 30.000f;
long last_mills;

void setup() {
  Serial.begin(115200);
  max7219.Begin();
  max7219.MAX7219_SetBrightness(90);
  pinMode(BUTTON_START, INPUT);
  pinMode(BUTTON_UP, INPUT);
  pinMode(BUTTON_DOWN, INPUT);
  //pinMode(BUZZER, OUTPUT);
  //digitalWrite(BUZZER, 1);
  last_mills = millis();
  mode = COUNTDOWN;
  countdown_reset_time = 30.0f;
  current_cd_time_left = countdown_reset_time;
}

void showTime()
{
  char buff[16];
  snprintf (buff, sizeof(buff), "%.3f0", current_cd_time_left);
  max7219.Clear();
  max7219.DisplayText(buff, RIGHT);
}

void showResetTime()
{
  char buff[16];
  snprintf (buff, sizeof(buff), "%.3f0", countdown_reset_time);
  Serial.write(buff);

  max7219.Clear();
  max7219.DisplayText(buff, RIGHT);
}

void stop_countdown()
{
  mode = STOP;
}

void adjust_start_time(float change)
{
  stop_countdown();
  countdown_reset_time += change;
  if(countdown_reset_time < 1.0f)
  {
    countdown_reset_time = 1.0f;
  }
  if(countdown_reset_time > 9999.0f)
  {
    countdown_reset_time = 9999.0f;
  }
}

void startcountdown()
{
  current_cd_time_left = countdown_reset_time;
  mode = COUNTDOWN;
}

long timeout_time = 0;
bool timeout_blink = false;
void startTimeout()
{
  mode = TIMEOUT;
  timeout_time = 0;
}

void countdown(long deltat)
{
  current_cd_time_left -= deltat / 1000.0f;
  if(current_cd_time_left <= 0.0f)
  {
    startTimeout();
    current_cd_time_left = 0.0f;
    return;
  }
  showTime();
}


void timeout(long deltat)
{
    timeout_time += deltat;
    if(mode == TIMEOUT)
    {
      if(timeout_time >= BLINKTIME)
      {
        timeout_blink = !timeout_blink;
        timeout_time = 0;
      }
    }
    if(mode == STOP)
    {
      if((timeout_time >= BLINKSTOPTIME && !timeout_blink) || (timeout_time >= BLINKTIME && timeout_blink))
      {
        timeout_blink = !timeout_blink;
        timeout_time = 0;
      }
    }
    if(timeout_blink)
    {
      
      if(mode == TIMEOUT)
      {
        max7219.Clear();
        max7219.DisplayText("   0.0000", RIGHT);
      }
      if(mode == STOP)
      {
        showResetTime();
      }
    }
    else
    {
      max7219.Clear();
    }
}

void checkbuttons(long deltat)
{
  static bool last_pressed_start  = false;
  static bool last_pressed_up     = false;
  static bool last_pressed_down   = false;

  bool pressed_start = digitalRead(BUTTON_START);
  if(pressed_start)
  {
    Serial.write("Pressed start\n");
  }

  static long pressed_start_time;

  bool pressed_up    = digitalRead(BUTTON_UP);
  if(pressed_up)
  {
    Serial.write("Pressed up\n");
  }
  static long pressed_up_time;
  static long pressed_up_delay;
  static int  pressed_up_count;

  bool pressed_down  = digitalRead(BUTTON_DOWN);
  if(pressed_down)
  {
    Serial.write("Pressed Down\n");
  }
  static long pressed_down_time;
  static long pressed_down_delay;
  static int  pressed_down_count;

  if(pressed_up)
  {
    if(last_pressed_up == false)
    {
      pressed_up_time = 0;
      pressed_up_delay = 500;
      pressed_up_count = 1;
      adjust_start_time(1.0f);
    }
    else
    {
      pressed_up_time += deltat;
      if(pressed_up_time >= pressed_up_delay)
      {
        pressed_up_time = 0;
        pressed_up_count += 1;
        adjust_start_time(1.0f);
        if(pressed_up_count >= 5)
        {
          pressed_up_delay = 100;
        }
        if(pressed_up_count > 20)
        {
          adjust_start_time(4.0f);
        }

      }
    }
  }

  if(pressed_down)
  {
    if(last_pressed_down == false)
    {
      pressed_down_time = 0;
      pressed_down_delay = 500;
      pressed_down_count = 1;
      adjust_start_time(-1.0f);
    }
    else
    {
      pressed_down_time += deltat;
      if(pressed_down_time >= pressed_down_delay)
      {
        pressed_down_time = 0;
        pressed_down_count += 1;
        adjust_start_time(-1.0f);
        if(pressed_down_count >= 5)
        {
          pressed_down_delay = 100;
        }
        if(pressed_down_count > 20)
        {
          adjust_start_time(-4.0f);
        }
      }
    }
  }

  if(pressed_start && last_pressed_start == false)
  {
    pressed_start_time = 0;
  }
  if(pressed_start)
  {
    pressed_start_time += deltat;
    if(pressed_start_time > 2000)
    {
      stop_countdown();
    }
  }

  if(last_pressed_start == false && pressed_start == true)
  {
    if(mode == COUNTDOWN || mode == STOP)
    {
      //Serial.println("button D4 released");
      startcountdown();
    }
    else
    {
       // if time ran out before button was pressed, go to pause mode instead of restarting
       // makes noticing running out of time just before pressing button easier.
      stop_countdown();
    }
  }
  if(last_pressed_start == true && pressed_start == false && mode == COUNTDOWN)
  {
    //restart timer after releasing button to make it feel more fair.
    startcountdown();
  }
  last_pressed_start = pressed_start;
  last_pressed_up    = pressed_up;
  last_pressed_down  = pressed_down;
}



void loop() {
  long new_mills = millis();
  int delta = new_mills - last_mills;
  if(delta >= 10)
  {
    last_mills = new_mills;
    checkbuttons(delta);
    switch(mode)
    {
      case COUNTDOWN:
        countdown(delta);
        break;
      case TIMEOUT:
        timeout(delta);
        break;
      case STOP:
        timeout(delta);
        break;
    }
  }
}
