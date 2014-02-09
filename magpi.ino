//BRANCH: EXPERI-MENTAL

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
#define DRAW_GAME    1
#define TEST_GAME   2
#define ZOOMING_GAME 3
#define OPTIONS_GAME 4

const int game_count = OPTIONS_GAME+1;
Game games[game_count] = {
  {menu,menu_init,0}
  ,
  {drawer,drawer_init,drawer_menu}
  ,
  {test,test_init,test_menu}
  ,
  {options,options_init,options_menu}
  ,
  {zooming,zooming_init,zooming_menu}
};

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

long dt;

uint8_t level;

void do_splash(const unsigned char *s) {
  if (splash) {
    splash = false;
    display.clearDisplay();
    display.drawBitmap(0,0,s,W,H,1);
    display.display();
    while(!pad_check()) {menu_check();};
  }
}

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
  display.print("Nav:");  
  display.write(27);  
  display.write(26);

  display.setCursor(0,32);
  display.print("Select:");
  display.print("A");

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

long ft,bt;

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
#define REPEAT_RATE 500

uint8_t current_option;
boolean draw = true;
void options() {
  if (draw) {
    display.clearDisplay();
    display.print("--Options--");
    display.setCursor(0,20);
    display.print(opts_name[current_option]);
    display.print(":");
    display.print(opts.values[current_option]-opts_min[current_option]);
    display.display();
  }
  unsigned long ct = millis();
  if (pad_check()) ft = 0;
  if (pad_hit != -1 && ct > ft) {
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
    if (current_option < 0) current_option = NUM_OPTIONS -1;
    else if (current_option >= NUM_OPTIONS) current_option = 0;
    draw = true;
  }
}
void options_menu() {
  display.setCursor(20,TITLE_Y);
  display.print("Options");
}
void options_init() {
  current_option=0;
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

uint8_t sc[H*(W/8+1)];
void screen_dump() {
  int i;
  Serial.print("Screen Dump:");
  Serial.print("\n\r");
  for(i = 0;i< H*(W/8+1);i++) sc[i]=0;
  i = 0;
  for (int y=0; y<H; y++) {
    for (int x=0; x<W; x++) {
      sc[i] |= display.getPixel(x,y) << 7-(x%8);
      if (x%8 == 7 || x%W == W-1) {
        Serial.print("0x");
        Serial.print(sc[i],HEX);
        Serial.print(",");
        i++;
      }
    }
    Serial.print("\n\r");
  }
  Serial.print("\n\r");
}

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
    case PAD_L+PAD_R:
      screen_dump();
      break;
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
  display.print("Drawer");
}
//---------------------------------------------------------------
//ZOOMING

uint8_t nextcube;
uint8_t cubespeed;
int spawnspeed;
uint8_t cubewh;
const uint8_t maxcubes = 19;
struct cube{uint8_t x; uint8_t z;};
struct cube cubes[maxcubes];
long ft2;

void drawscreen() {
  display.clearDisplay();
  for(uint8_t i = 0; i< maxcubes; i++) {
    cubewh = cubes[i].z / 4; //size based on z
    display.drawRect(cubes[i].x - (cubewh / 2),cubes[i].z - cubewh,cubewh,cubewh,1);
    display.fillTriangle(39,46, 45,46, 42,41, 1); //draws your character (a triangle)
    display.drawLine(0,12,84,12,1);  //draw horizon 0, 12 | 84, 12
  }
  display.display();
}

//random(1,84-3)

uint8_t randomx() {
  uint8_t last = cubes[(nextcube == 0) ? maxcubes - 1: nextcube - 1].x; //"if" that returns a value
  uint8_t x = random(1,84-3);
  while(abs(x - last) < 10) { //closest that cubes can form from the last cube
    x = random(1,84-3);
  }
  return x;
}

void init_cube() {
    cubes[nextcube].x = randomx();
    cubes[nextcube].z = 12; //horizon
    nextcube++;
    if(nextcube >= maxcubes) nextcube = 0;
}

void zooming_init() {
  nextcube = 0;
  ft = 0;
  ft2 = 0;
  cubespeed = 100;
  spawnspeed = 1000;
  cubewh = 2;
  display.clearDisplay();
  drawscreen();
//  for(uint8_t i = 0; i<= maxcubes; i++) {
//    init_cube(i);
//  }
}

void zooming() {
  long ct = millis();
  
  //spawn cubes loop
  if(ct >= ft2){
   
    init_cube();
    
    ft2 = ct + spawnspeed;
      }
      
  //advance cubes loop
  if(ct >= ft){
    
    for(uint8_t i = 0; i< maxcubes; i++) {
      cubes[i].z++;
    }

    ft = ct + cubespeed;
  }
  
  drawscreen();
}

void zooming_menu() {
  display.setCursor(20,TITLE_Y);
  display.print("Zooming");
}

//---------------------------------------------------------------
//TEST

const float pi = 3.14159265358979323846264338;

struct Cart {
 int8_t x;
 int8_t y;
};

struct Polar {
 int16_t angle;
 float radius;
};

Cart o = {42,24};
struct Cart polar2cart(struct Polar p, int16_t r = 0) {
  Cart c;
  
  float angler = deg2rad(p.angle+r);
  
  c.x = cos(angler) * p.radius;
  c.y = sin(angler) * p.radius;

  return c; 
}

float deg2rad(uint16_t deg) {
  return (2*pi)*deg/360;
}

Polar sp [] = {{45, 10},{135, 10},{225, 10},{315, 10}};

void drawsquare(uint16_t rot) {
  uint8_t sqc = 0;
  while (sqc <= 3) {
    Cart c1 = polar2cart(sp[sqc], rot);
    c1.x = c1.x + o.x;
    c1.y = c1.y + o.y;
    
    Cart c2 = polar2cart(sp[(sqc<3)?sqc+1:0], rot);
    c2.x = c2.x + o.x;
    c2.y = c2.y + o.y;
    
    
    display.drawLine(c1.x, c1.y, c2.x, c2.y, 1);
    sqc++;
  }
}

uint16_t rotv = 0;

void test_init() {
  Polar p = {45, 20};
  Cart c = polar2cart(p);  
  display.clearDisplay();
  display.display();

}

void test() {
  display.clearDisplay();
  drawsquare(rotv);
  pad_check();
  if(pad_hit == PAD_R) rotv += 7;
  else if(pad_hit == PAD_L) rotv -= 7;
//  else rotv = rotv;
    
  display.display();
  
}

void test_menu() {
  display.setCursor(11,TITLE_Y);
  display.print("Testing ^_^");
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
  setup_buttons();
  loadConfig();
  pinMode(A0, INPUT_PULLUP);
  analogWrite(9, opts.values[BRIGHTNESS]); // blPin is ocnnected to BL LED
  Serial.begin(115200);
  Serial.println("Game on!");
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




