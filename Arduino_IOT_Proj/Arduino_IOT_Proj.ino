#include<LiquidCrystal.h>

#define PIN_SWITCH 9
#define PIN_HORIZONTAL A0
#define PIN_VERTICAL A1

#define PIN_RS 12
#define PIN_ENABLE 11
#define PIN_D4 5
#define PIN_D5 4
#define PIN_D6 3
#define PIN_D7 2

#define SCREEN_COLS 16
#define SCREEN_ROWS 2

// Joystick Input
int xValue = 0;
int yValue = 0;
int bValue = 0;

// LCD
LiquidCrystal lcd(PIN_RS, PIN_ENABLE, PIN_D4, PIN_D5, PIN_D6, PIN_D7);

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
}

void loop()
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
