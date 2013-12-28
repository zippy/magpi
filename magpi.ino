#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(13, 11, 5, 7, 6);


typedef struct {
  void (*loop_fun)();
  void (*init_fun)();
  void (*menu_fun)();
} 
Game;

#define MENU_GAME     0
#define CATCHER_GAME  1
#define OPTIONS_GAME  2

const int game_count = 3;
Game games[game_count] = {
  {menu,menu_init,0},
  {catcher,catcher_init,catcher_menu},
  {options,options_init,options_menu},
};
int current_game = 1;

int game_choice;

void set_game(int game) {
  if (game == MENU_GAME && current_game != MENU_GAME) game_choice = current_game;
  current_game = game;
  (*games[current_game].init_fun)();
}


//---------------------------------------------------------------------
// UTILS
#define PAD_U 0x01
#define PAD_D 0x02
#define PAD_L 0x04
#define PAD_R 0x08
#define PAD_X 0x10
#define PAD_ALL 0x0F

int pad_hit;

bool pad_check() {
  if(Serial.available()) {
    char ch = Serial.read();
    if (ch=='a') pad_hit = PAD_L;
    else if (ch=='d') pad_hit = PAD_R;
    else if (ch=='s') pad_hit = PAD_D;
    else if (ch=='w') pad_hit = PAD_U;
    else if (ch=='r') pad_hit = PAD_R+PAD_L;
    else if (ch=='m') pad_hit = PAD_ALL;
    if (pad_hit == PAD_ALL) {
      set_game(MENU_GAME);
    }
    return true;
  }
  return false;
}

//---------------------------------------------------------------------
// MENU
#define TITLE_Y 15

void name() {
  display.clearDisplay();
  display.println("magpi");
  if (games[game_choice].menu_fun == 0) {
    display.print(game_choice);
  }
  else {
    (*games[game_choice].menu_fun)();
  }
  display.setCursor(0,30);
  display.print("Nav: ");  display.write(27);  display.write(26);

  display.setCursor(0,40);
  display.print("Select: ");display.write(25);

  display.display();
}

void menu() {
  if (pad_check()) {
    if (pad_hit == PAD_L || pad_hit == PAD_R) {
      game_choice+= (pad_hit == PAD_L) ? -1 : 1;
      if (game_choice >= game_count) {
        game_choice = 1;
      }
      else if (game_choice <= 0) {
        game_choice = game_count -1;
      }
      name();
    }
    else {
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
#define NUM_OPTIONS 2
#define CONTRAST 0
#define BRIGHTNESS 1
#define BACKLIGHT_PIN 9
char *opts_name[NUM_OPTIONS] = {"Contrast","Brightness"};
int opts[NUM_OPTIONS] = {58,255};
int opts_max[NUM_OPTIONS] = {75,255};
int opts_min[NUM_OPTIONS] = {45,0};
uint8_t current_option;
boolean draw = true;
void options() {
  if (draw) {
    display.clearDisplay();
    display.print("--Options--");
    display.setCursor(0,20);
    display.print(opts_name[current_option]);
    display.print(":");
    display.print(opts[current_option]);
    display.display();
  }
  if (pad_check()) {
    switch(pad_hit) {
    case PAD_R:
      current_option++;
      break;
    case PAD_L:
      current_option--;
      break;
    case PAD_U:
    case PAD_D:
      if (pad_hit == PAD_D && opts[current_option] > opts_min[current_option]) opts[current_option]--;
      if (pad_hit == PAD_U && opts[current_option] < opts_max[current_option]) opts[current_option]++;
      switch(current_option) {
      case CONTRAST:            
        display.setContrast(opts[CONTRAST]);
        break;
      case BRIGHTNESS:
        analogWrite(BACKLIGHT_PIN, opts[BRIGHTNESS]);
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

//---------------------------------------------------------------------
// CATCHER
const unsigned char PROGMEM catcher_bm[] =
{
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

#define FRAMES_PER_SECOND 10

#define W 80
#define H 48

#define cW 8
#define cH 3

#define R .4
#define MAX_AX 1.5
#define MAX_AY 1

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

long ft,bt;
int score;

uint8_t level,level_balls,frame;
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
  sprites[CATCHER].x = 40;
  sprites[CATCHER].y = 24;
  sprites[CATCHER].ax = 0;
  sprites[CATCHER].ay = 0;
}

/* pants */ void draw_catcher(void *s) {
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

void catcher_init() {
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
  wind = 2;
  ft = millis()+ ms_per_frame;
  level = 0;
  level_up = true;
  frame = level_balls = 0;
  next_ball_max = 10000;
  randomSeed(ft);
}

void move() {
  for(int i=0;i< sprite_count;i++) {

    // environmental factors: gravity wind and air resistance
    sprites[i].ay += gravity;
    if ((wind - sprites[i].ax) > 0) sprites[i].ax += .02;
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
        score--;
      }
    }
  }
}

long dt;
void catcher() {
  if (level_up) {
    reset_catcher();
    level++;
    level_up = false;
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("-- Catcher! --");
    display.setCursor(20,20);
    display.print("Level ");
    level_balls = level;
    display.print(level);
    display.display();
    delay(2000);
    if (next_ball_max > 3000) next_ball_max -= 500;
    bt = millis()+random(0,next_ball_max);
    gravity += .001;
  }
  dt = millis();
  if (dt > ft) {
    ft+= ms_per_frame;
    frame = (frame == FRAMES_PER_SECOND-1) ? 0 : frame+1;
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(score);
    //display.print(" ");   display.print(frame);
    //    display.print("ax:");display.print(sprites[0].ax);display.print(" ay:");display.print(sprites[0].ay);
    //    display.print("x:");display.print(sprites[0].x);display.print(" y:");display.print(sprites[0].y);
    for(int i=0;i< sprite_count;i++) {
      display.drawBitmap((int)sprites[i].x, (int)sprites[i].y,  sprites[i].bitmap, sprites[i].w, sprites[i].h, 1);
      if (sprites[i].draw_fun) (*sprites[i].draw_fun)(&sprites[i]);
    }
    display.drawLine(0,47,79,47,1);
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
    switch(pad_hit) {
    case PAD_R+PAD_L:
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
  }
}

void catcher_menu() {
  display.setCursor(20,TITLE_Y);

  display.print("Catcher!");
}

//---------------------------------------------------------------
void setup() {
  analogWrite(9, opts[BRIGHTNESS]); // blPin is ocnnected to BL LED
  Serial.begin(115200);
  Serial.println("Game on!");
  display.begin();

  display.setContrast(opts[CONTRAST]);
  set_game(MENU_GAME);
}

void loop() {
  (*games[current_game].loop_fun)();
}
