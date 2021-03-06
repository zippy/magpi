//BRANCH: MASTER

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Bounce.h>
#include <EEPROM.h>

//Sparkfun
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
// pin 2 - GND
// pin 1 - VCC

//Adafruit
// pin 7 - LCD reset (RST)
// pin 6 - LCD chip select (CS) 
// pin 5 - Data/Command select (D/C)
// pin 4 - Serial data out (DIN)
// pin 3 - Serial clock out (SCLK)
// pin 2 - VCC
// pin 1 - GND

//#define SERIAL_DEBUG

//Adafruit_PCD8544(int8_t SCLK, int8_t DIN, int8_t DC, int8_t CS, int8_t RST);
Adafruit_PCD8544 display = Adafruit_PCD8544(2,3, 4, 5, 6);
#define W 84
#define H 48

typedef struct {
  void (*loop_fun)();
  void (*init_fun)();
  void (*menu_fun)();
} 
Game;

#define MENU_GAME    0
#define CATCHER_GAME 1
#define DRAW_GAME    2
#define SNAKE_GAME   3
#define OPTIONS_GAME 4

const int game_count = OPTIONS_GAME+1;
Game games[game_count];

int current_game = 1;

int game_choice;
boolean splash;
boolean options_changed = false;

void set_game(int game) {
  if (options_changed) {
    saveConfig();
    options_changed = false;
  }

  if (game == MENU_GAME && current_game != MENU_GAME) game_choice = current_game;
  current_game = game;
  splash=true;
  (*games[current_game].init_fun)();
}

const unsigned char PROGMEM  sys_splash[] = {
  0x0,0xF0,0x0,0xF0,0xF,0xF0,0x0,0xF0,0x1F,0xFF,0x80,
  0x3,0xFC,0x3,0xFC,0x1F,0xFC,0x3,0xFC,0x1F,0xFF,0x80,
  0x7,0x9E,0x7,0x9E,0x18,0x1E,0x7,0x9E,0x0,0x60,0x0,
  0xE,0x7,0xE,0x7,0x1C,0x7,0xE,0x7,0x0,0x60,0x0,
  0xC,0x63,0xC,0x3,0x8E,0x3,0xC,0x3,0x0,0x60,0x0,
  0x1C,0x63,0x9C,0x3,0x87,0x83,0x9C,0x3,0x80,0x60,0x0,
  0x18,0x61,0x98,0x1,0x83,0xF9,0x98,0x1,0x80,0x60,0x0,
  0x18,0x61,0x98,0x1,0x80,0xF9,0x98,0x1,0x80,0x60,0x0,
  0x18,0x61,0x9C,0x1,0x80,0x1,0x98,0x3,0x80,0x60,0x0,
  0x18,0x61,0x8C,0x1,0x80,0x1,0x98,0x3,0x0,0x60,0x0,
  0x18,0x61,0x8E,0x1,0x80,0x3,0x98,0x7,0x0,0x60,0x0,
  0x18,0x61,0x87,0x81,0x80,0x7,0x18,0x1E,0x0,0x60,0x0,
  0x18,0x61,0x83,0xF9,0x9F,0xFF,0x19,0xFC,0x0,0x60,0x0,
  0x18,0x61,0x80,0xF9,0x9F,0xFC,0x19,0xF0,0x1F,0xFF,0x80,
  0x0,0x0,0x0,0x0,0x0,0x0,0x18,0x0,0x1F,0xFF,0x80,
  0x0,0x0,0x0,0x0,0x0,0x0,0x18,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x18,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x18,0x0,0x0,0x0,0x0,
};


//---------------------------------------------------------------------
// UTILS
#define PAD_U_PIN 7
#define PAD_U 0x01
#define PAD_D_PIN 8
#define PAD_D 0x02
#define PAD_L_PIN 11
#define PAD_L 0x04
#define PAD_R_PIN 10
#define PAD_R 0x08
#define PAD_A_PIN 12
#define PAD_A 0x10
#define PAD_B_PIN 13
#define PAD_B 0x20
#define PAD_M 0x40

#define NUM_BUTTONS 6
Bounce bouncers[] = {
  Bounce(),  Bounce(),  Bounce(),  Bounce(),  Bounce(),  Bounce()
};

const uint8_t pin_map[] = {
  PAD_U_PIN,PAD_D_PIN,PAD_L_PIN,PAD_R_PIN,PAD_A_PIN,PAD_B_PIN};
const uint8_t pad_map[] = {
  PAD_U,PAD_D,PAD_L,PAD_R,PAD_A,PAD_B};

int pad_hit;

boolean button_check(uint8_t button,uint8_t idx) {
  if (bouncers[idx].read()==LOW) {
    pad_hit |= button;
    return true;
  }
  else pad_hit &= ~button;
  return false;
}

boolean pad_check() {
  boolean ret_val = false;
  for(int i = 0;i<NUM_BUTTONS;i++) {
    if (bouncers[i].update()){
      ret_val |= button_check(pad_map[i],i);
    }
  }
  if (ret_val) return true;
#ifdef SERIAL_DEBUG
  if(Serial.available()) {
    char ch = Serial.read();
    if (ch=='a') pad_hit = PAD_L;
    else if (ch=='d') pad_hit = PAD_R;
    else if (ch=='s') pad_hit = PAD_D;
    else if (ch=='w') pad_hit = PAD_U;
    else if (ch=='h') pad_hit = PAD_A;
    else if (ch=='b') pad_hit = PAD_B;
    else if (ch=='m') pad_hit = PAD_M;
    else if (ch=='r') pad_hit = PAD_A+PAD_B;
    else if (ch=='p') pad_hit = PAD_L+PAD_R;
    if (pad_hit & PAD_M) {
      set_game(MENU_GAME);
    }
    return true;
  }
#endif
  return false;
}

//---------------------------------------------------------------------
// MENU
#define TITLE_Y 15

#define LOGO_X 44
#define LOGO_Y 9

const unsigned char PROGMEM logo_bm[] = {
  0x1C,0xE,0x1F,0x3,0x87,0xF0,
  0x22,0x11,0x10,0x84,0x40,0x80,
  0x49,0x20,0x88,0x48,0x20,0x80,
  0x49,0x20,0x87,0x48,0x20,0x80,
  0x49,0x20,0x80,0x48,0x20,0x80,
  0x49,0x10,0x80,0x88,0x40,0x80,
  0x49,0xE,0x9F,0xB,0x87,0xF0,
  0x0,0x0,0x0,0x8,0x0,0x0,
  0x0,0x0,0x0,0x8,0x0,0x0,
};

void menu_check() {
  if (analogRead(A0)< 1000) set_game(MENU_GAME);
}

void name() {
  display.clearDisplay();
  display.drawBitmap(1,1,logo_bm,LOGO_X,LOGO_Y,1);
  if (games[game_choice].menu_fun == 0) {
    display.print(game_choice);
  }
  else {
    (*games[game_choice].menu_fun)();
  }
  display.setCursor(0,40);
  display.print(F("Nav:"));  
  display.write(27);  
  display.write(26);

  display.setCursor(0,32);
  display.print(F("Select:"));
  display.print(F("A"));

  display.display();
}
void menu() {
  if (pad_check()) {
    if (pad_hit & (PAD_L+PAD_R)) {
      game_choice+= (pad_hit == PAD_L) ? -1 : 1;
      if (game_choice >= game_count) {
        game_choice = 1;
      }
      else if (game_choice <= 0) {
        game_choice = game_count -1;
      }
      name();
    }
    
    else if(pad_hit == PAD_A) {
      set_game(game_choice);
    }
  }
}

void menu_init() {
  name();
  display.display();
}

//---------------------------------------------------------------------
// OPTIONS

unsigned long ft,bt;

#define NUM_OPTIONS 2
#define CONTRAST 0
#define DEFAULT_CONTRAST 55
#define DEFAULT_BRIGHTNESS 255
#define BRIGHTNESS 1
#define BACKLIGHT_PIN 9

// ID of the settings block
#define CONFIG_VERSION "v10"

// Tell it where to store your config data in EEPROM
#define CONFIG_START 32

// Example settings structure
struct StoreStruct {
  // This is for mere detection if they are your settings
  char version[4];
  uint8_t values[NUM_OPTIONS];
} opts = {
  CONFIG_VERSION,
  {DEFAULT_CONTRAST,DEFAULT_BRIGHTNESS}
};

void loadConfig() {
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
    for (unsigned int t=0; t<sizeof(opts); t++)
      *((char*)&opts + t) = EEPROM.read(CONFIG_START + t);
}

void saveConfig() {
  for (unsigned int t=0; t<sizeof(opts); t++)
    EEPROM.write(CONFIG_START + t, *((char*)&opts + t));
}

const char * opts_name[NUM_OPTIONS] = {"Contrast","Brightness"};
const uint8_t opts_max[NUM_OPTIONS] = {75,255};
const uint8_t opts_min[NUM_OPTIONS] = {20,0};
#define REPEAT_RATE 250

uint8_t current_option;
boolean draw = true;
void options() {
  if (draw) {
    display.clearDisplay();
    display.print(F("--Options--"));
    display.setCursor(0,20);
    display.print(opts_name[current_option]);
    display.print(F(":"));
    display.print(opts.values[current_option]-opts_min[current_option]);
    display.display();
  }
  if (pad_check()) ft = 0;
  unsigned long ct = millis();
  if ((pad_hit != 0) && (ct > ft)) {
    ft = ct + REPEAT_RATE;
    switch(pad_hit) {
    case PAD_R:
      current_option++;
      break;
    case PAD_L:
      current_option--;
      break;
    case PAD_U:
    case PAD_D:
      if (pad_hit == PAD_D && opts.values[current_option] > opts_min[current_option]) opts.values[current_option]--;
      if (pad_hit == PAD_U && opts.values[current_option] < opts_max[current_option]) opts.values[current_option]++;
      options_changed = true;

      switch(current_option) {
      case CONTRAST:            
        display.setContrast(opts.values[CONTRAST]);
        break;
      case BRIGHTNESS:
        analogWrite(BACKLIGHT_PIN, opts.values[BRIGHTNESS]);
        break;
      }
    }
    if (current_option == 0xFF) current_option = NUM_OPTIONS -1;
    else if (current_option >= NUM_OPTIONS) current_option = 0;
    draw = true;
  }
}
void options_menu() {
  display.setCursor(20,TITLE_Y);
  display.print(F("Options"));
}
void options_init() {
  current_option=0;
}

//---------------------------------------------------------------------
// CATCHER
/*
0xE1,0xB0,0x98,0x8C,0x86,0xC3,0x0,0x0,0x0,0x0,
 0x9F,0xCF,0xE7,0xF3,0xF9,0xBD,0x0,0x0,0x0,0x0,
 0x81,0x81,0x81,0x81,0x81,0x81,0x0,0x0,0x0,0x0,
 0xE1,0xB1,0x99,0x8D,0x87,0xC3,0x0,0x0,0x0,0x0,
 0x9E,0xCF,0xE7,0xF3,0xF9,0xBC,0x0,0x0,0x0,0x0,
 0x80,0x80,0x80,0x80,0x80,0x80,0x0,0x0,0x0,0x0,
 0x80,0x80,0x80,0x80,0x80,0x80,0x0,0x0,0x0,0x0,
 0x80,0x80,0x80,0x80,0x80,0x80,0x0,0x0,0x0,0x0,
 
 0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
 0xC0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
 0xA0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
 0xA0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
 0x90,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
 0xF0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
 0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
 0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
 */

const unsigned char PROGMEM  catcher_splash[] = {

  0x73,0x11,0x1C,0x19,0xC4,0x68,0x0,0x0,0x0,0x0,0x0,
  0x94,0xBB,0xA4,0x22,0x4E,0x88,0x0,0x0,0x0,0x0,0x0,
  0x94,0x91,0x24,0x22,0x44,0x8E,0x0,0x0,0x7,0x80,0x0,
  0x73,0x11,0x1E,0x19,0xE4,0x6A,0x0,0x0,0xF,0xC0,0x0,
  0x10,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x78,0x78,0x0,
  0x72,0x60,0x0,0x0,0x0,0x0,0x0,0x0,0x78,0x78,0x0,
  0x2,0xAF,0x7,0x24,0x80,0x0,0x0,0x0,0xF,0xC0,0x0,
  0x0,0xCA,0x89,0x24,0x80,0x0,0x0,0x0,0x7,0x80,0x0,
  0x0,0x8A,0x89,0x24,0x3,0xC0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x60,0x7,0xB6,0x87,0xE0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x3C,0x3C,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x3C,0x3C,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x7,0xE0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x3,0xC0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x1E,0x0,0x78,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x1E,0x0,0x78,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x1E,0x0,0x78,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x1E,0x0,0x78,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0xF,0xFF,0xF0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x7,0xFF,0xE0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x55,0x0,0x0,0x0,0x13,0x0,0x0,
  0x0,0x0,0x0,0x0,0x2B,0x0,0x0,0x0,0x1C,0xE0,0x0,
  0x0,0x0,0x0,0x0,0x1A,0x0,0x0,0x0,0x10,0x20,0x0,
  0x0,0x0,0x0,0x0,0x14,0x0,0x0,0x0,0x13,0x20,0x0,
  0x0,0x0,0x0,0x0,0x8,0x0,0x0,0x0,0x1C,0xE0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x10,0x0,0x0,
  0x0,0x7F,0x0,0x0,0x8,0x0,0x0,0x0,0x10,0x0,0x0,
  0x0,0xFF,0x0,0x0,0x0,0x0,0x0,0x0,0x10,0x0,0x0,
  0x1,0xFE,0x7,0xC7,0xF8,0xFC,0xC6,0x7E,0x7E,0x18,0x0,
  0x1,0xE0,0xF,0xE3,0xF1,0xF8,0xC6,0x7C,0x7F,0x18,0x0,
  0x1,0xC0,0xC,0x60,0xC1,0xC0,0xC6,0x60,0x63,0x18,0x0,
  0x1,0xC0,0xC,0x60,0xC1,0x80,0xFE,0x78,0x63,0x18,0x0,
  0x1,0xC0,0xF,0xE0,0xC1,0x80,0xEE,0x78,0x7E,0x18,0x0,
  0x1,0xC0,0xE,0xE0,0xC1,0x80,0xC6,0x60,0x7E,0x18,0x0,
  0x1,0xC0,0xC,0x60,0xC1,0xC0,0xC6,0x60,0x67,0x18,0x0,
  0x1,0xE0,0xC,0x60,0xC1,0xF8,0xC6,0x7C,0x63,0x80,0x0,
  0x1,0xFE,0x8,0x20,0xC0,0xFC,0x82,0x7E,0x61,0xD8,0x0,
  0x0,0xFF,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x7F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF8,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,

};


const unsigned char PROGMEM catcher_bm[] = {
  B11000011,
  B11000011,
  B01111110,
};

const unsigned char PROGMEM ball_bm[] =
{
  B00011000,
  B01100110,
  B00011000,
};

const unsigned char PROGMEM flag0_bm[] = {
  0x80,0xC0,0xa0,0xA0,0x90,0xF0,0x80,0x80};
const unsigned char PROGMEM flag1_bm[] = {
  0xE1,0x9F,0x81,0xE1,0x9E,0x80,0x80,0x80};
const unsigned char PROGMEM flag2_bm[] = {
  0xB0,0xCF,0x81,0xB1,0xCF,0x80,0x80,0x80};
const unsigned char PROGMEM flag3_bm[] = {
  0x98,0xE7,0x81,0x99,0xE7,0x80,0x80,0x80};
const unsigned char PROGMEM flag4_bm[] = {
  0x8C,0xF3,0x81,0x8D,0xF3,0x80,0x80,0x80};
const unsigned char PROGMEM flag5_bm[] = {
  0x86,0xF9,0x81,0x87,0xF9,0x80,0x80,0x80};
const unsigned char PROGMEM flag6_bm[] = {
  0xC3,0xBD,0x81,0xC3,0xBC,0x80,0x80,0x80};

#define FLAG_COUNT 5
const unsigned char *flags_bm[] = {
  flag1_bm,flag2_bm,flag3_bm,flag4_bm,flag5_bm,flag6_bm};



#define FRAMES_PER_SECOND 10

#define cW 8
#define cH 3

#define R .2  // rocket impulse

#define MAX_AX 2  // max accellerations
#define MAX_AY 1.5

typedef struct sprite sprite;

struct sprite {
  float x;
  float y;
  float ax;
  float ay;
  int h;
  int w;
  const unsigned char  *bitmap;
  void (*draw_fun)(void *);
};

const int ms_per_frame = 1000/FRAMES_PER_SECOND;

#define MAX_SPRITES 10
#define CATCHER 0
sprite sprites[MAX_SPRITES];
int sprite_count = 1;
float gravity;
float air_resistance;
float wind;

int score;

uint8_t level,level_balls,balls_missed,frame;
boolean level_up;
int next_ball_max;

void reset_ball(int i) {
  sprites[i].x = random(0,W-cW-1);
  sprites[i].y = 0;
  sprites[i].ax = 0;
  sprites[i].ay = 0;
  sprites[i].w = cW;
  sprites[i].h = cH;
  sprites[i].bitmap = ball_bm;
  sprites[i].draw_fun = 0;
}

void add_ball() {
  reset_ball(sprite_count);
  sprite_count++;
}

void kill_ball(int b) {
  sprite_count--;
  for(int i=b;i<sprite_count;i++) {
    sprites[i] = sprites[i+1];
  }
  if (level_balls == 0 && sprite_count == 1) {
    level_up = true;
  }
}

void reset_catcher() {
  sprites[CATCHER].x = W/2;
  sprites[CATCHER].y = H/2;
  sprites[CATCHER].ax = 0;
  sprites[CATCHER].ay = 0;
}

/* pants */void draw_catcher(void *s) {
  int i = frame &1;
  switch (pad_hit) {
  case PAD_D:
    switch(i) {
    case 0:
      display.drawPixel(((sprite *)s)->x+2,((sprite *)s)->y+3,1);
      display.drawPixel(((sprite *)s)->x+5,((sprite *)s)->y+3,1);
      display.drawPixel(((sprite *)s)->x+3,((sprite *)s)->y+4,1);
      display.drawPixel(((sprite *)s)->x+4,((sprite *)s)->y+3,1);
      display.drawPixel(((sprite *)s)->x+3,((sprite *)s)->y+5,1);
      display.drawPixel(((sprite *)s)->x+4,((sprite *)s)->y+4,1);
      break;
    case 1:
      display.drawPixel(((sprite *)s)->x+2,((sprite *)s)->y+4,1);
      display.drawPixel(((sprite *)s)->x+5,((sprite *)s)->y+4,1);
      display.drawPixel(((sprite *)s)->x+3,((sprite *)s)->y+3,1);
      display.drawPixel(((sprite *)s)->x+4,((sprite *)s)->y+4,1);
      display.drawPixel(((sprite *)s)->x+3,((sprite *)s)->y+4,1);
      display.drawPixel(((sprite *)s)->x+4,((sprite *)s)->y+5,1);
      break;
    }
    break;
  case PAD_L:
    display.drawPixel(((sprite *)s)->x-1-i,((sprite *)s)->y+0,1);
    display.drawPixel(((sprite *)s)->x-2-i,((sprite *)s)->y+1,1);
    display.drawPixel(((sprite *)s)->x-1-i,((sprite *)s)->y+2,1);
    break;
  case PAD_R:
    display.drawPixel(((sprite *)s)->x+8+i,((sprite *)s)->y+0,1);
    display.drawPixel(((sprite *)s)->x+9+i,((sprite *)s)->y+1,1);
    display.drawPixel(((sprite *)s)->x+8+i,((sprite *)s)->y+2,1);
    break;
  }
}

void do_splash(const unsigned char *s) {
  if (splash) {
    splash = false;
    display.clearDisplay();
    display.drawBitmap(0,0,s,W,H,1);
    display.display();
    while(!pad_check()) {menu_check();};
  }
}

void catcher_init() {
  do_splash(catcher_splash);
  score = 0;
  sprite_count = 1;
  reset_catcher();
  sprites[CATCHER].w = cW;
  sprites[CATCHER].h = cH;
  sprites[CATCHER].bitmap = catcher_bm;
  sprites[CATCHER].draw_fun = &draw_catcher;

  add_ball();

  gravity = .005;
  air_resistance = .005;
  wind = 0;
  ft = millis()+ ms_per_frame;
  level = 0;
  balls_missed = 0;
  level_up = true;
  frame = level_balls = 0;
  next_ball_max = 10000;
  randomSeed(ft);
}

void move() {
  for(int i=0;i< sprite_count;i++) {

    // environmental factors: gravity wind and air resistance
    sprites[i].ay += gravity;
    if (wind > 0) {
      if ((wind - sprites[i].ax) > 0) sprites[i].ax += wind/50;
    }
    else if (wind < 0) {
      if ((wind - sprites[i].ax) < 0) sprites[i].ax += wind/50;
    }
    if (sprites[i].ax > 0) sprites[i].ax -= air_resistance;
    if (sprites[i].ax < 0) sprites[i].ax += air_resistance;

    // sprite inertia wrapping and bouncing
    sprites[i].x += sprites[i].ax;
    sprites[i].y += sprites[i].ay;
    int w = sprites[i].w;
    int h = sprites[i].h;
    float x = sprites[i].x;
    float y = sprites[i].y;
    if (x < 0) sprites[i].x = (W-w)-.1;
    if (x >= W-w) sprites[i].x = 0;
    if (y < 0) sprites[i].y = (H-h)-.1;
    if (y >= H-h) {
      sprites[i].ay /= 2.5; 
      sprites[i].ay *= -1;
      sprites[i].y += sprites[i].ay;
      if (y>= H-h) {
        sprites[i].y = H-h-1;
      }
    }

    // ball collisions
    if (i > 0) {
      float x0 = sprites[0].x;
      float y0 = sprites[0].y;

      // with the catcher
      if ((x >= x0-3 && x <= x0+3 && y>=y0-1 && y < y0+3)) {
        score++;
        kill_ball(i);
      }
      if ((x > x0-5 && x < x0-3 && y>=y0-2 && y < y0+1) ||
        (x > x0+3 && x < x0+5 && y>=y0-2 && y < y0+1)) {
        float by = sprites[i].ay;
        float cy = sprites[0].ay;
        sprites[i].ay += (cy-by);
        sprites[0].ay += (by-cy);
        sprites[i].y += sprites[i].ay;
        sprites[0].y += sprites[0].ay;
        float bx = sprites[i].ax;
        float cx = sprites[0].ax;
        sprites[i].ax += (cx-bx);
        sprites[0].ax += (bx-cx);
        sprites[i].x += sprites[i].ax;
        sprites[0].x += sprites[0].ax;
      }
      // with the ground
      if (y>=H-cH-2 && abs(sprites[i].ay) < .1) {
        kill_ball(i);
        balls_missed++;
        score--;
      }
    }
  }
}

float flap = 0;
long dt;
void catcher() {
  if (level_up) {
    reset_catcher();
    if (balls_missed == 0) level++;
    level_up = false;
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(F("-- Catcher! --"));
    if (balls_missed > 0) {
      display.setCursor(20,10);
      display.print(F("Missed "));
      display.print(balls_missed);
    }
    balls_missed = 0;
    display.setCursor(20,20);
    display.print(F("Level "));
    level_balls = level;
    display.print(level);
    display.display();
    delay(2000);
    ft = millis()+ ms_per_frame;

    if (next_ball_max > 3000) next_ball_max -= 500;
    bt = millis()+random(0,next_ball_max);
    gravity += .001;
    if (level > 1) {
      wind = level * .05 + random(0,10)/20.0;
      if (random(0,2)) wind *= -1;
    }
    else wind = 0;
  }
  dt = millis();
  if (dt > ft) {
    switch(pad_hit) {
    case PAD_A+PAD_B:
      catcher_init(); 
      break;
    case PAD_L:
      if (sprites[CATCHER].ax < MAX_AX) sprites[CATCHER].ax+= R;
      break;
    case PAD_R:
      if (sprites[CATCHER].ax > -MAX_AX) sprites[CATCHER].ax-= R;
      break;
    case PAD_U:
      if (sprites[CATCHER].ay < MAX_AY) sprites[CATCHER].ay+= R;
      break;
    case PAD_D:
      if (sprites[CATCHER].ay > -MAX_AY) sprites[CATCHER].ay-= R;
      break;
    }


    ft+= ms_per_frame;
    frame = (frame == FRAMES_PER_SECOND-1) ? 0 : frame+1;
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(score);
    //display.print(" ");   display.print(wind);
    //    display.print("ax:");display.print(sprites[0].ax);display.print(" ay:");display.print(sprites[0].ay);
    //    display.print("x:");display.print(sprites[0].x);display.print(" y:");display.print(sprites[0].y);
    for(int i=0;i< sprite_count;i++) {
      display.drawBitmap((int)sprites[i].x, (int)sprites[i].y,  sprites[i].bitmap, sprites[i].w, sprites[i].h, 1);
      if (sprites[i].draw_fun) (*sprites[i].draw_fun)(&sprites[i]);
    }
    if (wind != 0) {
      display.drawBitmap(5,H-8,flags_bm[(int)flap],8,8,1);
      if (frame&1) flap++;
    }
    else {
      display.drawBitmap(5,H-8,flag0_bm,8,8,1);
    }
    if (flap > FLAG_COUNT) flap = 0;
    display.drawLine(0,H-1,W-1,H-1,1);
    display.display();
    move();
  }

  // add balls in randomly according to lvel
  if (dt > bt && level_balls > 0) {
    level_balls--;
    bt = millis()+random(0,next_ball_max);
    add_ball();
  }

  if (pad_check()) {
    if (pad_hit == PAD_A+PAD_B)
      catcher_init(); 
  }
}

void catcher_menu() {
  display.setCursor(20,TITLE_Y);

  display.print(F("Catcher!"));
}

//---------------------------------------------------------------
// DRAWER

#define BLINK_RATE 300

int px,py;
boolean pd;
uint8_t p_state;


const unsigned char PROGMEM  drawer_splash[] = {
  /*
  
   
   
   0xE1,0xB0,0x98,0x8C,0x86,0xC3,0x0,0x0,0x0,0x0,
   0x9F,0xCF,0xE7,0xF3,0xF9,0xBD,0x0,0x0,0x0,0x0,
   0x81,0x81,0x81,0x81,0x81,0x81,0x0,0x0,0x0,0x0,
   0xE1,0xB1,0x99,0x8D,0x87,0xC3,0x0,0x0,0x0,0x0,
   0x9E,0xCF,0xE7,0xF3,0xF9,0xBC,0x0,0x0,0x0,0x0,
   0x80,0x80,0x80,0x80,0x80,0x80,0x0,0x0,0x0,0x0,
   0x80,0x80,0x80,0x80,0x80,0x80,0x0,0x0,0x0,0x0,
   0x80,0x80,0x80,0x80,0x80,0x80,0x0,0x0,0x0,0x0,  
   */
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x7F,0x3,0xF8,0x1,0xC0,0xE3,0x3,0xF8,0x3F,0x80,0x0,
  0x8,0x80,0x44,0x2,0x20,0x31,0x0,0x80,0x4,0x40,0x0,
  0x8,0x80,0xF8,0x3,0xE0,0x15,0x0,0xF0,0xF,0x80,0x0,
  0x8,0x80,0x44,0x2,0x20,0x1B,0x0,0x80,0x4,0x40,0x0,
  0x1F,0x0,0x44,0x1E,0x60,0x11,0x0,0xFF,0x4,0x40,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0xF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF8,0x0,
  0xA,0xD0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xC,0x0,
  0xD,0x50,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xF,0x0,
  0xA,0xD0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xC,0x0,
  0xF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF8,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x8,0x84,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x8,0x8E,0x0,
  0x1,0x4,0x78,0xE,0x0,0x0,0x0,0x8,0x8,0x95,0x0,
  0x2,0x8A,0x45,0x91,0x0,0x0,0x0,0x14,0xC8,0x84,0x0,
  0x4,0x44,0x45,0xA0,0x80,0x0,0x0,0x22,0xC8,0x84,0x0,
  0x4,0x4D,0x78,0x22,0xA0,0x0,0x0,0x22,0xF,0x84,0x0,
  0x7,0xD6,0x45,0xA1,0xC0,0x0,0x0,0x3E,0xC7,0x4,0x0,
  0x4,0x52,0x45,0x90,0x80,0x0,0x0,0x22,0xC2,0x15,0x0,
  0x4,0x4D,0x78,0xE,0x0,0x0,0x0,0x22,0x2,0xE,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x4,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1F,0xC0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x4,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x15,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0xE,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x4,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x3F,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x4,0x0,0x22,0x80,0x0,0x0,0x3C,0x0,0x0,0x0,
  0x1,0xA,0x11,0xA2,0x40,0x0,0x0,0x22,0xDF,0xFE,0x0,
  0x2,0x4,0x9,0xA3,0xC0,0x0,0x0,0x22,0xD5,0xA0,0x0,
  0x7,0xCD,0x7C,0x20,0x40,0x0,0x0,0x3C,0x1A,0xA0,0x0,
  0x2,0x16,0x9,0xA0,0x40,0x0,0x0,0x22,0xD5,0xA0,0x0,
  0x1,0x12,0x11,0xA0,0x40,0x0,0x0,0x22,0xDF,0xFE,0x0,
  0x0,0xD,0x0,0x20,0x40,0x0,0x0,0x3C,0x0,0x0,0x0,
  0x0,0x0,0x0,0x3F,0xC0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,

};

uint8_t kc;

void drawer_init() {
  do_splash(drawer_splash);
  kc = 0;
  px = W/2;
  py = H/2;
  pd = false;

  display.clearDisplay();
  //  display.drawBitmap(0,0,catcher_splash,W,H,1);

//  display.drawBitmap(1,1,logo_bm,LOGO_X,LOGO_Y,1);
//  display.drawBitmap(0,3,sys_splash,W,18,1);

  display.display();
  // display.drawBitmap(0,hbsam0,xx,W,7,1);
  ft = millis() + BLINK_RATE;
  p_state = 0;
}

#ifdef SERIAL_DEBUG
void screen_dump() {
  uint8_t sc;
  Serial.print(F("Screen Dump:"));
  Serial.print(F("\n\r"));
  sc = 0;
  for (int y=0; y<H; y++) {
    for (int x=0; x<W; x++) {
      sc |= display.getPixel(x,y) << 7-(x%8);
      if (x%8 == 7 || x%W == W-1) {
        Serial.print(F("0x"));
        Serial.print(sc,HEX);
        Serial.print(F(","));
        sc=0;
      }
    }
    Serial.print(F("\n\r"));
  }
  Serial.print(F("\n\r"));
}
#endif

void doKC() {
  analogWrite(BACKLIGHT_PIN, 0);
  delay(200);
  analogWrite(BACKLIGHT_PIN, 255);
  delay(200);
  analogWrite(BACKLIGHT_PIN, 0);
  delay(200);  
  analogWrite(BACKLIGHT_PIN, 255);
  delay(200);
  analogWrite(BACKLIGHT_PIN, 0);
  delay(200);
  analogWrite(BACKLIGHT_PIN, opts.values[BRIGHTNESS]);
}


void drawer() {
  dt = millis();
  if (!pd && dt > ft) {
    ft += BLINK_RATE;    
    display.drawPixel(px,py, display.getPixel(px,py) ? 0:1 );
    display.display();
  }
  if (pad_check()) {
    
//    <DO NOT TRY TO FIGURE OUT WHAT THIS CODE DOES!>
boolean done = 0;
    if(pad_hit == PAD_U && ((kc == 0) || (kc == 1))) {
      kc++;
      done = 1;}
    if(pad_hit == PAD_D && ((kc == 2) || (kc == 3))) {
      kc++;
      done = 1;}
    if(pad_hit == PAD_L && ((kc == 4) || (kc == 6))) {
      kc++;
      done = 1;}
    if(pad_hit == PAD_R && ((kc == 5) || (kc == 7))) {
      kc++;
      done = 1;}
    if(pad_hit == PAD_B && (kc == 8)) {
      kc++; 
      done = 1;}
    if(pad_hit == PAD_A && (kc == 9)) {
      kc++;
      done = 1;}
    if(pad_hit == PAD_M && (kc == 10)) {
      kc++;
      done = 1;}
    if(done != 1) kc = 0;
    if(kc == 11) doKC();
//    </DO NOT TRY TO FIGURE OUT WHAT THIS CODE DOES!>

    if (!pd && pad_hit & PAD_L+PAD_R+PAD_D+PAD_U) {
      display.drawPixel(px,py,p_state);
    }
    switch(pad_hit) {
    case PAD_A+PAD_B:
      drawer_init();
      break;
    case PAD_L:
      if (px == 0) px=W-1;
      else px--;
      break;
    case PAD_R:
      if (px == W-1) px=0;
      else px++;
      break;
    case PAD_D:
      if (py == H-1) py=0;
      else py++;
      break;
    case PAD_U:
      if (py == 0) py=H-1;
      else py--;
      break;
    case PAD_A:
      pd = pd ? false : true;
      break;
    case PAD_B:
      if (!pd) p_state = 0;
      break;
 #ifdef SERIAL_DEBUG
    case PAD_L+PAD_R:
      screen_dump();
      break;
#endif
    }
    if (!pd && pad_hit & PAD_L+PAD_R+PAD_D+PAD_U) {
      p_state = display.getPixel(px,py);
    }
    if (pd) {
      display.drawPixel(px,py,1);
      if (pad_hit == PAD_A) p_state = 1;
    }

    display.display();
  }
}

void drawer_menu() {
  display.setCursor(20,TITLE_Y);
  display.print(F("Drawer"));
}
//---------------------------------------------------------------
//UBER SNAKE

const int max_length = 20;
typedef struct{
  uint8_t x;
  uint8_t y;
} Point;
Point segs[max_length];

int dir;
int snake_speed;
int seg_life;
int growing;

int cur_seg;
int d_seg;
uint8_t applex;
uint8_t appley;

void newapple() {
  applex = random(1,83);
  appley = random(1,47);
  display.drawPixel(applex,appley,1);
}

void snake_init() {
  growing = false;
  snake_speed = 100;
  seg_life = 3;
  level = 1;
  dir = PAD_R;
  cur_seg = 2;
  d_seg = 0;
  
  segs[0].x = 42;
  segs[1].x = 43;
  segs[2].x = 44;

  segs[0].y = 24;
  segs[1].y = 24;
  segs[2].y = 24;

  display.clearDisplay();
  display.display();
  
  newapple();
  
  ft = millis()+snake_speed;
  
  randomSeed(ft);
  
  px = 44;
  py = 24;

}

void snake() {
  
  if((px == applex) && (py == appley)) { //apple is eaten
    growing = true;
    newapple();
  }
    
  if(pad_check()) dir = pad_hit;
  
  //future time/speed
  if(millis() >= ft) {
    ft = millis()+snake_speed; //can be more efficient (refactor millis)
    

    if(dir == PAD_U)py--;
    else if(dir == PAD_R)px++;
    else if(dir == PAD_D)py++;
    else if(dir == PAD_L)px--;
    
    display.drawPixel(px,py,1);
    
    cur_seg++;
    if(cur_seg > max_length - 1) cur_seg = 0;
    segs[cur_seg].x = px;
    segs[cur_seg].y = py;
    
    display.drawPixel(segs[d_seg].x,segs[d_seg].y,0);
    
    if(!growing) d_seg++;
    else growing = false;
    
    if(d_seg > max_length - 1) d_seg = 0;
    
    display.display();
  }
}

void snake_menu() {
  display.setCursor(13,TITLE_Y);
  display.print(F("Uber Snake"));
}


//---------------------------------------------------------------

void setup_buttons() {
  for(int i =0;i< NUM_BUTTONS;i++) {
    pinMode(pin_map[i], INPUT);      // Push-Button On Bread Board
    digitalWrite(pin_map[i], HIGH);  // Turn on internal Pull-Up Resistor
    bouncers[i].interval(15);
    bouncers[i].attach(pin_map[i]);
  }
}

void setup() {
  games[0] =  {menu, menu_init, 0};
  games[1] =  {catcher,catcher_init,catcher_menu};
  games[2] =  {drawer,drawer_init,drawer_menu};
  games[3] =  {snake,snake_init,snake_menu};
  games[4] =  {options,options_init,options_menu};
  
  setup_buttons();
  loadConfig();
  pinMode(A0, INPUT_PULLUP);
  analogWrite(9, opts.values[BRIGHTNESS]); // blPin is ocnnected to BL LED
#ifdef SERIAL_DEBUG
  Serial.begin(115200);
  Serial.println(F("Game on!"));
#endif
  display.begin();
  display.setContrast(opts.values[CONTRAST]);
  display.clearDisplay();
  display.drawBitmap(0,3,sys_splash,W,18,1);
  display.display();
  delay(1000);

  set_game(MENU_GAME);
}

void loop() {
  (*games[current_game].loop_fun)();
  menu_check();
}
