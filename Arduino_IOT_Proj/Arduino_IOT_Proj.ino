#include<LiquidCrystal.h>
#include<LedControl.h>

#pragma region CONSTANTS

#define PIN_SWITCH 9
#define PIN_HORIZONTAL A0
#define PIN_VERTICAL A1

#define PIN_RS 12
#define PIN_ENABLE 11
#define PIN_D4 5
#define PIN_D5 4
#define PIN_D6 3
#define PIN_D7 2

#define DIN 10
#define CS 7
#define CLK 13

#define SCREEN_COLS 16
#define SCREEN_ROWS 2

#define EASY 1
#define MEDIUM 2
#define HARD 3

#pragma region SIDESCROLLER_CONST

#define PIN_AUTOPLAY 1
#define SPRITE_RUN1 1
#define SPRITE_RUN2 2
#define SPRITE_JUMP 2
#define SPRITE_JUMP_UPPER 3         // Use the '.' character for the head
#define SPRITE_JUMP_LOWER 4
#define SPRITE_TERRAIN_EMPTY ' '      // User the ' ' character
#define SPRITE_TERRAIN_SOLID 5
#define SPRITE_TERRAIN_SOLID_RIGHT 6
#define SPRITE_TERRAIN_SOLID_LEFT 7

#define HERO_HORIZONTAL_POSITION 1    // Horizontal position of hero on screen

#define TERRAIN_WIDTH 16
#define TERRAIN_EMPTY 0
#define TERRAIN_LOWER_BLOCK 1
#define TERRAIN_UPPER_BLOCK 2

#define HERO_POSITION_OFF 0          // Hero is invisible
#define HERO_POSITION_RUN_LOWER_1 1  // Hero is running on lower row (pose 1)
#define HERO_POSITION_RUN_LOWER_2 2  //                              (pose 2)

#define HERO_POSITION_JUMP_1 3       // Starting a jump
#define HERO_POSITION_JUMP_2 4       // Half-way up
#define HERO_POSITION_JUMP_3 5       // Jump is on upper row
#define HERO_POSITION_JUMP_4 6       // Jump is on upper row
#define HERO_POSITION_JUMP_5 7       // Jump is on upper row
#define HERO_POSITION_JUMP_6 8       // Jump is on upper row
#define HERO_POSITION_JUMP_7 9       // Half-way down
#define HERO_POSITION_JUMP_8 10      // About to land

#define HERO_POSITION_RUN_UPPER_1 11 // Hero is running on upper row (pose 1)
#define HERO_POSITION_RUN_UPPER_2 12 //     

#pragma endregion SIDESCROLLER_CONST

#pragma region SNAKE

#define SECTOR_WIDTH 5
#define SECTOR_HEIGHT 8
#define SECTORS_PER_ROW 16
#define SECTORS_PER_COLUMN 2
#define BOARD_WIDTH (SECTORS_PER_ROW * SECTOR_WIDTH)
#define BOARD_HEIGHT (SECTORS_PER_COLUMN * SECTOR_HEIGHT)

#pragma endregion SNAKE

#pragma endregion CONSTANTS

// Joystick Input
int xValue = 0;
int yValue = 0;
int bValue = 0;

// LCD
LiquidCrystal lcd(PIN_RS, PIN_ENABLE, PIN_D4, PIN_D5, PIN_D6, PIN_D7);

LedControl lc(DIN, CLK, CS, 0);

void setup()
{
  lcd.begin(SCREEN_COLS, SCREEN_ROWS); 
  Serial.begin(9600);
  pinMode(PIN_SWITCH, INPUT_PULLUP);
  pinMode(PIN_HORIZONTAL, INPUT);
  pinMode(PIN_VERTICAL, INPUT);

  mazeSetup();
  snakeSetup();
}

#pragma region MAIN_LOOP

void loop()
{
  sidescrollerMainLoop(MEDIUM);
  //snakeLoop();
}

#pragma endregion MAIN_LOOP

#pragma region SIDESCROLLER

static char terrainUpper[TERRAIN_WIDTH + 1];
static char terrainLower[TERRAIN_WIDTH + 1];
static bool buttonPushed = false;
static bool lastButtonState = false;

void initializeGraphics(){
  static byte graphics[] = {
    // Run position 1
    B01110,
    B11111,
    B11000,
    B11111,
    B11111,
    B11111,
    B01010,
    B01000,
    // Run position 2
    B01110,
    B11111,
    B11000,
    B11111,
    B11111,
    B11111,
    B01010,
    B00010,
    // Jump
    B00000,
    B00000,
    B00000,
    B00000,
    B01110,
    B11111,
    B11000,
    B11111,
    // Jump lower
    B11111,
    B11111,
    B01010,
    B01010,
    B00000,
    B00000,
    B00000,
    B00000,
    // Ground
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    // Ground right
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    // Ground left
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
  };
  int i;
  // Skip using character 0, this allows lcd.print() to be used to
  // quickly draw multiple characters
  for (i = 0; i < 7; ++i) {
    lcd.createChar(i + 1, &graphics[i * 8]);
  }
  for (i = 0; i < TERRAIN_WIDTH; ++i) {
    terrainUpper[i] = SPRITE_TERRAIN_EMPTY;
    terrainLower[i] = SPRITE_TERRAIN_EMPTY;
  }
}

int getEmptyTerrainLength(int difficulty)
{
  switch (difficulty)
  {
    case EASY:
      return 15;
    case MEDIUM:
      return 10;
    case HARD:
      return 5;
    default:
      return 10;
  }
}

void sidescrollerMainLoop(int difficulty) {

  static byte heroPos = HERO_POSITION_RUN_LOWER_1;
  static byte newTerrainType = TERRAIN_EMPTY;
  static byte newTerrainDuration = 1;
  static bool playing = false;
  static bool blink = false;
  static unsigned int distance = 0;
  
  const int baseEmptyTerrainLength = getEmptyTerrainLength(difficulty);

  checkButton();

  if (!playing) {
    drawHero((blink) ? HERO_POSITION_OFF : heroPos, terrainUpper, terrainLower, distance >> 3);
    if (blink) {
      lcd.setCursor(0,0);
      lcd.print("Press Start");
    }
    delay(250);
    blink = !blink;
    if (buttonPushed) {

      initializeGraphics();
      heroPos = HERO_POSITION_RUN_LOWER_1;
      playing = true;
      buttonPushed = false;
      distance = 0;
    }
    return;
  }

  // Shift the terrain to the left
  advanceTerrain(terrainLower, newTerrainType == TERRAIN_LOWER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);
  advanceTerrain(terrainUpper, newTerrainType == TERRAIN_UPPER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);
  
  // Make new terrain to enter on the right
  if (--newTerrainDuration == 0) {
    if (newTerrainType == TERRAIN_EMPTY) {
      newTerrainType = (random(3) == 0) ? TERRAIN_UPPER_BLOCK : TERRAIN_LOWER_BLOCK;
      newTerrainDuration = 2 + random(10);
    } else {
      newTerrainType = TERRAIN_EMPTY;
      newTerrainDuration = baseEmptyTerrainLength + random(5);
    }
  }
    
  if (buttonPushed) {
    if (heroPos <= HERO_POSITION_RUN_LOWER_2) 
      heroPos = HERO_POSITION_JUMP_1;
    buttonPushed = false;
  }  

  if (drawHero(heroPos, terrainUpper, terrainLower, distance >> 3)) {
    playing = false; // The hero collided with something. Too bad.
  } else {
    //advance hero
    if (heroPos == HERO_POSITION_RUN_LOWER_2 || heroPos == HERO_POSITION_JUMP_8) {
      heroPos = HERO_POSITION_RUN_LOWER_1;
    } else if ((heroPos >= HERO_POSITION_JUMP_3 && heroPos <= HERO_POSITION_JUMP_5) && terrainLower[HERO_HORIZONTAL_POSITION] != SPRITE_TERRAIN_EMPTY) {
      heroPos = HERO_POSITION_RUN_UPPER_1;
    } else if (heroPos >= HERO_POSITION_RUN_UPPER_1 && terrainLower[HERO_HORIZONTAL_POSITION] == SPRITE_TERRAIN_EMPTY) {
      heroPos = HERO_POSITION_JUMP_5;
    } else if (heroPos == HERO_POSITION_RUN_UPPER_2) {
      heroPos = HERO_POSITION_RUN_UPPER_1;
    } else {
      ++heroPos;
    }
    ++distance;
    
    digitalWrite(PIN_AUTOPLAY, terrainLower[HERO_HORIZONTAL_POSITION + 2] == SPRITE_TERRAIN_EMPTY ? HIGH : LOW);
  }
  delay(100);

}


// Slide the terrain to the left in half-character increments
//
void advanceTerrain(char* terrain, byte newTerrain){
  for (int i = 0; i < TERRAIN_WIDTH; ++i) {
    char current = terrain[i];
    char next = (i == TERRAIN_WIDTH-1) ? newTerrain : terrain[i+1];
    switch (current){
      case SPRITE_TERRAIN_EMPTY:
        terrain[i] = (next == SPRITE_TERRAIN_SOLID) ? SPRITE_TERRAIN_SOLID_RIGHT : SPRITE_TERRAIN_EMPTY;
        break;
      case SPRITE_TERRAIN_SOLID:
        terrain[i] = (next == SPRITE_TERRAIN_EMPTY) ? SPRITE_TERRAIN_SOLID_LEFT : SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_RIGHT:
        terrain[i] = SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_LEFT:
        terrain[i] = SPRITE_TERRAIN_EMPTY;
        break;
    }
  }
}

bool drawHero(byte position, char* terrainUpper, char* terrainLower, unsigned int score) {
  bool collide = false;
  char upperSave = terrainUpper[HERO_HORIZONTAL_POSITION];
  char lowerSave = terrainLower[HERO_HORIZONTAL_POSITION];
  byte upper, lower;
  switch (position) {
    case HERO_POSITION_OFF:
      upper = lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_LOWER_1:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN1;
      break;
    case HERO_POSITION_RUN_LOWER_2:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN2;
      break;
    case HERO_POSITION_JUMP_1:
    case HERO_POSITION_JUMP_8:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_JUMP;
      break;
    case HERO_POSITION_JUMP_2:
    case HERO_POSITION_JUMP_7:
      upper = SPRITE_JUMP_UPPER;
      lower = SPRITE_JUMP_LOWER;
      break;
    case HERO_POSITION_JUMP_3:
    case HERO_POSITION_JUMP_4:
    case HERO_POSITION_JUMP_5:
    case HERO_POSITION_JUMP_6:
      upper = SPRITE_JUMP;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_UPPER_1:
      upper = SPRITE_RUN1;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_UPPER_2:
      upper = SPRITE_RUN2;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
  }
  if (upper != ' ') {
    terrainUpper[HERO_HORIZONTAL_POSITION] = upper;
    collide = (upperSave == SPRITE_TERRAIN_EMPTY) ? false : true;
  }
  if (lower != ' ') {
    terrainLower[HERO_HORIZONTAL_POSITION] = lower;
    collide |= (lowerSave == SPRITE_TERRAIN_EMPTY) ? false : true;
  }
  
  byte digits = (score > 9999) ? 5 : (score > 999) ? 4 : (score > 99) ? 3 : (score > 9) ? 2 : 1;
  
  // Draw the scene
  terrainUpper[TERRAIN_WIDTH] = '\0';
  terrainLower[TERRAIN_WIDTH] = '\0';
  char temp = terrainUpper[16-digits];
  terrainUpper[16-digits] = '\0';
  lcd.setCursor(0,0);
  lcd.print(terrainUpper);
  terrainUpper[16-digits] = temp;  
  lcd.setCursor(0,1);
  lcd.print(terrainLower);
  
  lcd.setCursor(16 - digits,0);
  lcd.print(score);

  terrainUpper[HERO_HORIZONTAL_POSITION] = upperSave;
  terrainLower[HERO_HORIZONTAL_POSITION] = lowerSave;
  return collide;
}

void checkButton()
{
  static const int threshhold = 600;
  int joystickState = analogRead(PIN_VERTICAL);
  if (joystickState > threshhold && lastButtonState == false)
  {
    buttonPushed = true;
    lastButtonState = true;
  }
  else 
  {
    buttonPushed = false;
    lastButtonState = false;
  }
}

#pragma endregion SIDESCROLLER


#pragma region Maze

bool buttonPressed = false;


int cursor_col = 0; // Coloana inițială
int cursor_row = 2; // Rândul inițial

// Matricea stării curente a LED-urilor
int CurrentState[8][8] = {0};

bool game_won = false; // Indicator pentru câștig

void mazeSetup()
{
  lc.shutdown(0, false);
  lc.setIntensity(0, 15);
  lc.clearDisplay(0);
  
  CurrentState[cursor_row][cursor_col] = 1;
  lc.setLed(0, cursor_row, cursor_col, true); // Aprindem LED-ul direct
}

// Matrici de mutare
int Move_up[8][8] = {
  {0,0,0,0,0,0,0,0},
  {1,1,1,0,1,0,1,1},
  {1,1,1,1,0,0,0,1},
  {0,1,1,1,0,0,0,0},
  {1,0,1,1,0,1,1,1},
  {1,1,0,0,0,1,1,1},
  {1,0,0,0,0,1,0,1},
  {0,0,1,0,0,0,0,1}
};


int Move_down[8][8] = {
  {1,1,1,0,1,0,1,1},
  {1,1,1,1,0,0,0,1},
  {0,1,1,1,0,0,0,0},
  {1,0,1,1,0,1,1,1},
  {1,1,0,0,0,1,1,1},
  {1,0,0,0,0,1,1,1},
  {0,0,1,0,0,0,0,1},
  {0,0,0,0,0,0,0,0}
};


int Move_left[8][8] = {
  {0,1,0,0,1,1,0,1},
  {0,0,0,0,1,1,1,0},
  {0,0,1,0,0,1,1,1},
  {0,1,0,0,1,1,0,1},
  {0,0,1,0,1,0,0,0},
  {0,0,1,1,1,0,0,0},
  {0,1,1,1,1,1,1,1},
  {0,1,1,1,1,1,1,0}
};


int Move_right[8][8] = {
  {1,0,0,1,1,0,1,0},
  {0,0,0,1,1,1,0,1},
  {0,1,0,0,1,1,1,0},
  {1,0,0,1,1,0,1,0},
  {0,1,0,1,0,0,0,0},
  {0,1,1,1,0,0,0,0},
  {1,1,1,1,1,1,1,0},
  {1,1,1,1,1,1,0,0}
};


void Move_in_the_maze() {
  if (game_won) return; // Oprește mutările dacă jocul e câștigat

  xValue = analogRead(PIN_HORIZONTAL);
  yValue = analogRead(PIN_VERTICAL);

  int new_row = cursor_row;
  int new_col = cursor_col;


 if (xValue < 450 && Move_up[cursor_row][cursor_col] == 1 && cursor_row > 0) { // Stânga → Sus
    new_row--;
  } else if (yValue < 450 && Move_right[cursor_row][cursor_col] == 1 && cursor_col < 7) { // Sus → Dreapta
    new_col++;
  } else if (xValue > 570 && Move_down[cursor_row][cursor_col] == 1 && cursor_row < 7) { // Dreapta → Jos
    new_row++;
  } else if (yValue > 570 && Move_left[cursor_row][cursor_col] == 1 && cursor_col > 0) { // Jos → Stânga
    new_col--;
  }

  // Actualizăm poziția doar dacă s-a schimbat
  if (new_row != cursor_row || new_col != cursor_col) {
    // Stingem LED-ul de pe poziția curentă
    CurrentState[cursor_row][cursor_col] = 0; // Deactivăm LED-ul curent
    lc.setLed(0, cursor_row, cursor_col, false); // Aprindem LED-ul direct

    // Aprindem LED-ul pe noua poziție
    CurrentState[new_row][new_col] = 1; // Activăm LED-ul pe noua poziție
    lc.setLed(0, new_row, new_col, true); // Aprindem LED-ul direct

    // Actualizăm cursorul
    cursor_row = new_row;
    cursor_col = new_col;
    // Aprindem și LED-ul de pe noua poziție
    CurrentState[cursor_row][cursor_col] = 1;
  }

  // Verifică dacă poziția curentă este cea finală
  if (cursor_row == 1 && cursor_col == 7) {
    game_won = true;
    Serial.println("Felicitări, ai câștigat!");
    // Afișăm mesajul pe LCD
    lcd.clear();
    lcd.setCursor(0, 0); // Setează cursorul la începutul liniei 0
    lcd.print("Felicitari!");
    delay(1000);
    lcd.setCursor(0, 1); // Linia 1
    lcd.print("Ai reusit!");

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            lc.setLed(0, row, col, true);  // Aprinde LED-ul pe fiecare poziție
        }
    }
  }

}

void mazeLoop() {
  Move_in_the_maze();
  delay(200); // Întârziere pentru control mai precis
}

#pragma endregion Maze

#pragma region SNAKE

extern LiquidCrystal lcd;

const int JOYSTICK_X_PIN = A0;
const int JOYSTICK_Y_PIN = A1;

const short MIN_DRAW_WAIT = 60;

bool left_just_pressed = false;
bool right_just_pressed = false;
bool up_just_pressed = false;
bool down_just_pressed = false;

struct point_t {
  char x;
  char y;
};

struct snake_node_t {
  struct snake_node_t *next;
  struct snake_node_t *prev;
  struct point_t pos;
};

enum Direction {
  LEFT = 1,
  DOWN = LEFT << 1,
  RIGHT = LEFT << 2,
  UP = LEFT << 3,
};

struct snake_node_t* snake_head;
Direction curr_direction;
struct point_t apple_pos;

void generate_apple() {
  apple_pos.x = random(BOARD_WIDTH);
  apple_pos.y = random(BOARD_HEIGHT);
}

void init_character(byte* character) {
  for(int i = 0; i < SECTOR_HEIGHT; i++) {
  character[i] = B00000;
  }
}

struct snake_node_t* add_snake_part() {
  struct snake_node_t* new_part = (struct snake_node_t*) malloc(sizeof(struct snake_node_t));
  struct snake_node_t* last = snake_head->next;
  snake_head->next = new_part;
  last->prev = new_part;
  new_part->next = last;
  new_part->prev = snake_head;
  return new_part;
}


void move_snake() {
  struct point_t last_pos = snake_head->pos;
  struct snake_node_t* curr_node = snake_head->prev;

  //Move head
  switch(curr_direction) {
    case LEFT:
      if(snake_head->pos.x == 0) {
        snake_head->pos.x = BOARD_WIDTH - 1;
      } else {
        snake_head->pos.x--;
      }
      break;
    case DOWN:
      if(snake_head->pos.y == (BOARD_HEIGHT - 1)) {
        snake_head->pos.y = 0;
      } else {
        snake_head->pos.y++;
      }
      break;
    case RIGHT:
      if(snake_head->pos.x == (BOARD_WIDTH - 1)) {
        snake_head->pos.x = 0;
      } else {
        snake_head->pos.x++;
      }
      break;
    case UP:
      if(snake_head->pos.y == 0) {
        snake_head->pos.y = BOARD_HEIGHT - 1;
      } else {
        snake_head->pos.y--;
      }
      break;
  }

  //Move body
  while(curr_node != snake_head) {
    struct point_t temp = curr_node->pos;
    curr_node->pos = last_pos;
    last_pos = temp;
    curr_node = curr_node->prev;
  }
}

void draw_snake() {
  byte sectors[SECTORS_PER_ROW][SECTORS_PER_COLUMN][SECTOR_HEIGHT];
  for(int i = 0; i < SECTORS_PER_COLUMN; i++) {
    for(int j = 0; j < SECTORS_PER_ROW; j++) {
      init_character(sectors[j][i]);
    }
  }

  //Apple drawing
  byte sec_x = apple_pos.x / SECTOR_WIDTH;
  byte sec_y = apple_pos.y / SECTOR_HEIGHT;
  sectors[sec_x][sec_y][apple_pos.y - (sec_y * SECTOR_HEIGHT)] |= (B10000 >> (apple_pos.x - (sec_x * SECTOR_WIDTH)));

  struct snake_node_t* curr_node = snake_head;

  do{
    struct point_t p = curr_node->pos;
    sec_x = p.x / SECTOR_WIDTH;
    sec_y = p.y / SECTOR_HEIGHT;
    sectors[sec_x][sec_y][p.y - (sec_y * SECTOR_HEIGHT)] |= (B10000 >> (p.x - (sec_x * SECTOR_WIDTH)));
    curr_node = curr_node->prev;
  }while(curr_node != snake_head);

  byte curr_sec = 0;

  for(int i = 0; i < SECTORS_PER_COLUMN; i++) {
    for(int j = 0; j < SECTORS_PER_ROW; j++) {
      bool draw_sec = false;
      for(int k = 0; k < SECTOR_HEIGHT; k++) {
        if(sectors[j][i][k]) {
          draw_sec = true;
          break;
        }
      }

      if(draw_sec) {
        lcd.createChar(curr_sec, sectors[j][i]);
        lcd.setCursor(j, i);
        lcd.write(byte(curr_sec));
        curr_sec++;
      } else {
        lcd.setCursor(j, i);
        lcd.write(" ");
      }
    }
  }
}

void snakeSetup() {
  randomSeed(analogRead(0));
  byte character[SECTOR_HEIGHT];

  snake_head = (struct snake_node_t*) malloc(sizeof(struct snake_node_t));
  snake_head->pos.x = 5;
  snake_head->pos.y = 5;
  snake_head->prev = snake_head;
  snake_head->next = snake_head;
  struct snake_node_t* body1 = add_snake_part();
  struct snake_node_t* body2 = add_snake_part();
  body1->pos.x = 4;
  body1->pos.y = 5;
  body2->pos.x = 3;
  body2->pos.y = 5;
  curr_direction = RIGHT;
  generate_apple();
  draw_snake();
}

short time_since_last_draw = 0;
unsigned long last_update = 0;


void update_input() {
    int xValue = analogRead(JOYSTICK_X_PIN);
    int yValue = analogRead(JOYSTICK_Y_PIN);

    right_just_pressed = (xValue < 300);  
    left_just_pressed = (xValue > 700); 
    
    down_just_pressed = (yValue < 300);   
    up_just_pressed = (yValue > 700);   

    if (left_just_pressed && curr_direction != RIGHT) {
        curr_direction = LEFT;
    } else if (right_just_pressed && curr_direction != LEFT) {
        curr_direction = RIGHT;
    } else if (up_just_pressed && curr_direction != DOWN) {
        curr_direction = UP;
    } else if (down_just_pressed && curr_direction != UP) {
        curr_direction = DOWN;
    }
}

bool check_collision() {
    struct snake_node_t* curr_node = snake_head->prev;
    while (curr_node != snake_head) {
        if (snake_head->pos.x == curr_node->pos.x && snake_head->pos.y == curr_node->pos.y) {
            return true;
        }
        curr_node = curr_node->prev;
    }
    return false; 
}


void snakeLoop() {
    update_input();

    unsigned long time = millis();
    unsigned long elapsed = time - last_update;
    last_update = time;
    time_since_last_draw += elapsed;

    if (time_since_last_draw >= MIN_DRAW_WAIT) {
        move_snake();

        if (check_collision()) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Game Over!");
            while (true); 
        }

        if (snake_head->pos.x == apple_pos.x && snake_head->pos.y == apple_pos.y) {
            generate_apple();
            add_snake_part();
        }
        draw_snake();
        time_since_last_draw = 0;
    }
}

#pragma endregion SNAKE

#pragma region HANGMAN

String easyWords[] = {"CASA", "MASA", "PARA", "MARE", "LUNA", "APA", "FOC", "PANA", "VACA", "MAMA", "TATA", "TARE", "COPT"};
String mediumWords[] = {"PISICA", "MASINA", "SCOALA", "STRADA", "FRUNZA", "PUTERE", "OPOSUM", "MUSTATA"};
String hardWords[] = {"CALENDAR", "TELEFON", "COMPUTER", "BIBLIOTECA", "CRACIUN", "FACULTATE", "FLORARIE"};
String currentWord;
String displayWord;
char currentLetter = 'A';
int lives = 6;
boolean gameOver = false;
int difficulty = 0;
boolean needUpdate = false;

void hangmanSetup() {
  lcd.begin(16, 2);
  pinMode(PIN_SWITCH, INPUT_PULLUP);
  randomSeed(analogRead(A2));
  selectDifficulty();
}

void hangmanLoop() {
  if(!gameOver) {
    if(handleJoystick() || needUpdate) {
      updateDisplay();
      needUpdate = false;
    }
    checkButton();
    delay(50);
  } else {
    if(digitalRead(PIN_SWITCH) == LOW) {
      delay(200);
      selectDifficulty();
    }
  }
}

void selectDifficulty() {
  lcd.clear();
  lcd.print("Dificultate:");
  difficulty = 0;
  updateDifficultyDisplay();
  
  while(digitalRead(PIN_SWITCH) == HIGH) {
    int yValue = analogRead(PIN_VERTICAL);
    if(yValue > 800) {
      difficulty = (difficulty + 1) % 3;
      updateDifficultyDisplay();
      delay(200);
    }
    else if(yValue < 200) {
      difficulty = (difficulty - 1 + 3) % 3;
      updateDifficultyDisplay();
      delay(200);
    }
  }
  delay(500);
  startGame();
}

void updateDifficultyDisplay() {
  lcd.setCursor(0, 1);
  switch(difficulty) {
    case 0: lcd.print("Usor          "); break;
    case 1: lcd.print("Mediu         "); break;
    case 2: lcd.print("Greu          "); break;
  }
}

void startGame() {
  int randomIndex = random(4);
  switch(difficulty) {
    case 0: currentWord = easyWords[randomIndex]; break;
    case 1: currentWord = mediumWords[randomIndex]; break;
    case 2: currentWord = hardWords[randomIndex]; break;
  }
  
  displayWord = "";
  for(int i = 0; i < currentWord.length(); i++) {
    displayWord += "_";
  }
  
  lives = 6;
  gameOver = false;
  currentLetter = 'A';
  needUpdate = true;
}

boolean handleJoystick() {
  int xValue = analogRead(PIN_HORIZONTAL);
  boolean changed = false;
  
  if(xValue > 800) {
    currentLetter++;
    if(currentLetter > 'Z') currentLetter = 'A';
    changed = true;
    delay(200);
  }
  else if(xValue < 200) {
    currentLetter--;
    if(currentLetter < 'A') currentLetter = 'Z';
    changed = true;
    delay(200);
  }
  
  return changed;
}

void hangmanCheckButton() {
  if(digitalRead(PIN_SWITCH) == LOW) {
    checkLetter();
    needUpdate = true;
    delay(200);
  }
}

void checkLetter() {
  boolean found = false;
  for(int i = 0; i < currentWord.length(); i++) {
    if(currentWord[i] == currentLetter) {
      displayWord[i] = currentLetter;
      found = true;
    }
  }
  
  if(!found) {
    lives--;
    if(lives <= 0) {
      gameOver = true;
      showGameOver();
    }
  }
  
  if(displayWord.equals(currentWord)) {
    gameOver = true;
    showWin();
  }
}

void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Lives: ");
  lcd.print(lives);
  lcd.print(" ");
  lcd.print(currentLetter);
  
  lcd.setCursor(0, 1);
  lcd.print(displayWord);
}

void showGameOver() {
  lcd.clear();
  lcd.print("Game Over!");
  lcd.setCursor(0, 1);
  lcd.print(currentWord);
}

void showWin() {
  lcd.clear();
  lcd.print("Ai castigat!");
  lcd.setCursor(0, 1);
  lcd.print(currentWord);
}

#pragma endregion HANGMAN
