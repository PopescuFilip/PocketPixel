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

#define SNAKE_MAX_LENGTH 16
#define SNAKE_START_LENGTH 5
#define FOOD_SYMBOL 'F'
#define SNAKE_SYMBOL 'S'
#define EMPTY_SYMBOL ' '

#pragma endregion SNAKE

#pragma endregion CONSTANTS

// Joystick Input
int xValue = 0;
int yValue = 0;
int bValue = 0;

// LCD
LiquidCrystal lcd(PIN_RS, PIN_ENABLE, PIN_D4, PIN_D5, PIN_D6, PIN_D7);

LedControl lc(DIN, CLK, CS, 0);

// Text Input Variables

String chars[5] = {
  "_ABCDEF","GHIJKLM","NOPQRST","UVWXYZ","=-"
};

String savedWords[100] = {};

int viewWordIndex = 0;
int savedWordsIndex = 0;
int curGroup = 0;
int curChar;
bool hasSelected = false;
String myText = "";

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
  static bool playOld = false;

  if (playOld)
  {
    oldMainLoop();
  }
  else
  {
    //sidescrollerMainLoop();
    //mazeLoop();
    //snakeLoop();
  }
}

void oldMainLoop()
{
  xValue = analogRead(PIN_HORIZONTAL);  
  yValue = analogRead(PIN_VERTICAL);  
  bValue = digitalRead(PIN_SWITCH);

  lcd.clear();
  lcd.setCursor(0,0);   
  if(!hasSelected)
  {
    lcd.print(chars[curGroup]);
  }
  else 
  {
    lcd.print(chars[curGroup][curChar]);
  }

  lcd.setCursor(0,1);
  lcd.print(">" + myText + "<");

  if(xValue <= 10)
  {
      if(!hasSelected)
      {
        curGroup = (curGroup + 1) % 5;
      }
      else
      {
        int curLen = chars[curGroup].length();
        curChar = (curChar + 1) % curLen;
      }
  }
  else if(xValue >= 1010)
  {
      if(!hasSelected)
      {
        curGroup = curGroup > 0 ? (curGroup - 1) % 5 : 4;
      }
      else
      {
        int curLen = chars[curGroup].length();
        curChar = curChar > 0 ? (curChar - 1) % curLen : curLen - 1;
      }
  }
  
  if(yValue >= 1010)
  {
      if(hasSelected)
      {
        hasSelected = false;
      }
      else 
      {
        //View saved words
        if(viewWordIndex > savedWordsIndex)
        {
          viewWordIndex = 0;
        }
        myText = savedWords[viewWordIndex];
        viewWordIndex++;
      }
  }
  else if(yValue <= 10)
  {
      if(!hasSelected)
      {
        hasSelected = true;
        curChar = 0;
      }
      else 
      {
        if(chars[curGroup][curChar] == '-' && myText.length() > 0)
        {
          myText.remove(myText.length()-1);
        }
        else if(chars[curGroup][curChar] == '_' && myText.length() < 14)
        {
          myText += ' ';
        }
        else if(chars[curGroup][curChar] == '=' && myText.length() > 0)
        {
          savedWords[savedWordsIndex++] = myText;
          myText = "";
          curGroup = 0;
        }
        else if(myText.length() < 14)
        {
          myText += chars[curGroup][curChar];
        }

        hasSelected = false;
      }
  } 

  if(!bValue)
  {
    Serial.println(myText + " ");
    myText = "";
    curGroup = 0;
    hasSelected = false;
  }
  
  //  Serial.print(xValue,DEC);
  // Serial.print(",");
  // Serial.print(yValue,DEC);
  // Serial.print(",");
  // Serial.print(bValue,DEC);
  // Serial.print("\n");
  

  delay(250);
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

void sidescrollerMainLoop(){

  static byte heroPos = HERO_POSITION_RUN_LOWER_1;
  static byte newTerrainType = TERRAIN_EMPTY;
  static byte newTerrainDuration = 1;
  static bool playing = false;
  static bool blink = false;
  static unsigned int distance = 0;
  
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
      newTerrainDuration = 10 + random(10);
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
  int buttonState = digitalRead(PIN_SWITCH);
  if (buttonState == HIGH && buttonState != lastButtonState)
  {
    buttonPushed = true;
  }
  else 
  {
    buttonPushed = false;
  }
  lastButtonState = buttonState;
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

// Enumeration for directions
enum Direction { UP, DOWN, LEFT, RIGHT };

struct Position {
    int x, y;
};

Position snake[SNAKE_MAX_LENGTH];
Position food;
int snake_length = SNAKE_START_LENGTH;
bool snake_alive = true;
bool snake_game_active = false;
Direction snake_direction = Direction::UP;

// Function to check if food is generated on the snake's body
bool isFoodOnSnake() {
    for (int i = 0; i < snake_length; i++) {
        if (snake[i].x == food.x && snake[i].y == food.y) {
            return true;
        }
    }
    return false;
}

void snakeSetup() {
    randomSeed(analogRead(0)); // Seed for random number generation
    food.x = random(1, SCREEN_COLS - 1);
    food.y = random(1, SCREEN_ROWS - 1);
    for (int i = 0; i < snake_length; i++) {
        snake[i].x = SCREEN_COLS / 2;
        snake[i].y = SCREEN_ROWS / 2 + i;
    }
    snake_direction = Direction::UP;
    snake_game_active = true;
    snake_alive = true;
}

void drawSnakeGame() {
    lcd.clear();
    for (int i = 0; i < snake_length; i++) {
        lcd.setCursor(snake[i].x, snake[i].y);
        lcd.write(SNAKE_SYMBOL);
    }
    lcd.setCursor(food.x, food.y);
    lcd.write(FOOD_SYMBOL);
}

void moveSnake() {
    Position next = snake[0];

    // Determine the next position of the snake
    switch (snake_direction) {
        case Direction::UP:    next.y--; break;
        case Direction::DOWN:  next.y++; break;
        case Direction::LEFT:  next.x--; break;
        case Direction::RIGHT: next.x++; break;
    }

    // Check for collision with screen boundaries
    if (next.x < 0) next.x = SCREEN_COLS - 1;
    if (next.y < 0) next.y = SCREEN_ROWS - 1;
    if (next.x >= SCREEN_COLS) next.x = 0;
    if (next.y >= SCREEN_ROWS) next.y = 0;

   // Check for collision with the snake's body
    for (int i = 0; i < snake_length; i++) {
        if (snake[i].x == next.x && snake[i].y == next.y) {
            snake_alive = false;
            return;
        }
    }

    // Move the snake
    for (int i = snake_length - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }
    snake[0] = next;

    // Check if the snake eats the food
    if (snake[0].x == food.x && snake[0].y == food.y) {
        if (snake_length < SNAKE_MAX_LENGTH) {
            snake[snake_length] = snake[snake_length - 1];
            snake_length++;
        }
        // Respawn the food
        do {
            food.x = random(1, SCREEN_COLS - 1);
            food.y = random(1, SCREEN_ROWS - 1);
        } while (isFoodOnSnake());
    }
}

void snakeLoop() {
    if (!snake_game_active) return;

    xValue = analogRead(PIN_HORIZONTAL);
    yValue = analogRead(PIN_VERTICAL);

    // Read joystick direction and update snake's direction
    Direction new_direction = snake_direction;
    if (xValue < 450) new_direction = Direction::UP;
    else if (xValue > 570) new_direction = Direction::DOWN;
    else if (yValue < 450) new_direction = Direction::LEFT;
    else if (yValue > 570) new_direction = Direction::RIGHT;

    // Check if the new direction is valid
    if (!((snake_direction == Direction::UP && new_direction == Direction::DOWN) ||
          (snake_direction == Direction::DOWN && new_direction == Direction::UP) ||
          (snake_direction == Direction::LEFT && new_direction == Direction::RIGHT) ||
          (snake_direction == Direction::RIGHT && new_direction == Direction::LEFT))) {
        snake_direction = new_direction;
    }

    if (snake_alive) {
        moveSnake();
        drawSnakeGame();
    } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Game Over");

        // Reset the game after Game Over
        if (digitalRead(PIN_SWITCH) == LOW) { 
            snakeSetup();
        }
    }

    delay(200);// Delay to control game speed
}

#pragma endregion SNAKE
