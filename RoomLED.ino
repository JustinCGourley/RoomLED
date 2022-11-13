#include "FastLED.h"

#define LED_BRIGHTNESS 255
#define LED_NUM 300
CRGB leds[LED_NUM];

#define LED_STRIP_PIN 11
//#define LED_PIN_R 3
//#define LED_PIN_G 4
//#define LED_PIN_B 5
#define BUTTON_PIN 6

#define ON_SWITCH_PIN A0
//#define DIAL_PIN A1

//defaults
//shimmer
const int shimmer_max = 255;
const byte shimmer_max_speed = 20;
const byte shimmer_min_speed = 15;
const byte shimmer_threshhold = 1;

CRGB baseColor = CRGB(0,0,0);
CRGB shimmerTo[LED_NUM];
CRGB shimmerMove[LED_NUM];
//moving fade logic vars
#define MOVING_FADE_TIME 5
#define MOVING_FADE_FADEOUT 1
#define MOVING_FADE_FADEOUT_TIME 0
#define MOVING_FADE_MIN 0.01
#define MOVING_FADE_MIN_VAL 3
int movingFadeNum = 0;
unsigned long movingFadeTime;
bool movingFadeUp = true;
//fade color logic vars
byte fadecolorState = 6;
unsigned long fadeColorTime = 0;
#define FADE_COLOR_UPDATE_TIME 5000
//static colors
#define STATIC_COLOR_COUNT 7 //this needs to be the number of elements in static_colors
CRGB static_colors[] = {CRGB::Blue, CRGB::Green, CRGB::Cyan, CRGB::Purple, CRGB::Orange, CRGB::Red, CRGB::White};
//basic logic vars
bool buttonDown = false;
byte colorState = 0;
byte lightState = 1; //0 - static | 1 - shimmer | 2 - moving fade
#define LIGHT_STATE_COUNT 3
unsigned long startHold;
//timing vars
unsigned long timeHold = 0;
#define SHIMMER_UPDATE_TIME 100
//strobe
bool isStrobe = false;
bool strobeOn = false;
#define STROBE_TIME 500
unsigned long strobeTime = 0;


void setup() {
  Serial.begin(9600);
//  pinMode(LED_PIN_R, OUTPUT);
//  pinMode(LED_PIN_G, OUTPUT);
//  pinMode(LED_PIN_B, OUTPUT);

//  pinMode(DIAL_PIN, INPUT);
  pinMode(ON_SWITCH_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);
  
  pinMode(LED_STRIP_PIN, OUTPUT);
  FastLED.addLeds<NEOPIXEL, LED_STRIP_PIN>(leds, LED_NUM); 
  FastLED.setBrightness(LED_BRIGHTNESS);
  baseColor = static_colors[0];
  setStrip();

  for (int i = 0; i < LED_NUM; i++)
  {
    updateShimmerTarget(i);
  }
}

void loop() {

  int button = digitalRead(BUTTON_PIN);
  checkButton(button);
  int serialRead;
  serialRead = Serial.read();


  if (serialRead == 48) buttonPress();
  else if (serialRead == 49) buttonHold();
  else if (serialRead == 50) {setStrobe();}
  if (!buttonDown && startHold != 0)
  {
    checkButtonPress();
  }

  if (colorState == STATIC_COLOR_COUNT)
  {
    fadeColorUpdate();
  }

  if (isStrobe) 
  { 
      checkStrobe();
  }
  else
  {
    switch(lightState)
    {
      case 0://static
        if (colorState == STATIC_COLOR_COUNT) setupStrip();
      break;
      case 1://shimmer
        if (millis() - timeHold >= SHIMMER_UPDATE_TIME)
        {
          shimmer();
          timeHold = millis();
        }
      break;
      case 2://moving fade
        movingFade();
      break;
      default:
      break;
    }
  
  }
  
}

void checkStrobe()
{
  if (isStrobe && (millis() - strobeTime >= STROBE_TIME))
  {
    CRGB color;
    if (!strobeOn)
    {
      strobeOn = true; 
      color = getBaseColor(0);
    }
    else
    {
      strobeOn = false;
      color = CRGB(0,0,0);
    }
    strobeTime = millis();
    for (int i = 0; i < LED_NUM; i++)
    {
      leds[i] = color;
    }
    FastLED.show();
  }
  else if (!isStrobe && strobeOn) 
  {
    for (int i = 0; i < LED_NUM; i++)
    {
      leds[i] = getBaseColor(i);
    }
  }
}

void movingFade()
{
  if (millis() - timeHold >= MOVING_FADE_FADEOUT_TIME)
  {
    for (int i = 0; i < LED_NUM; i++)
    {
      fadeOutColor(i, -MOVING_FADE_FADEOUT, MOVING_FADE_MIN);
    }
    timeHold = millis();
  }
  

  if (movingFadeTime == 0) movingFadeTime = millis();
  if (millis() - movingFadeTime >= MOVING_FADE_TIME)
  {
    if (movingFadeUp) { movingFadeNum++; }
    else { movingFadeNum--; }
    if (movingFadeNum >= LED_NUM || movingFadeNum <= 0)
    {
      if (movingFadeNum >= LED_NUM) movingFadeNum = LED_NUM-1;
      else if (movingFadeNum <= 0) movingFadeNum = 0;
      movingFadeUp = !movingFadeUp;
    }
    else if (movingFadeNum <= 0)
    {
      movingFadeNum = 0;
    }
    movingFadeTime = millis();  

    setLEDToBaseColor(movingFadeNum);
  }
  
  FastLED.show();
}

void setLEDToBaseColor(int index)
{
  leds[index] = getBaseColor(index);
}

void fadeOutColor(int index, int amount, float minValPercentage)
{
  CRGB color = getBaseColor(index);
  if (leds[index].r > (float)(color.r) * minValPercentage && leds[index].r >= MOVING_FADE_MIN_VAL) leds[index].r+=amount;
  if (leds[index].g > (float)(color.g) * minValPercentage && leds[index].g >= MOVING_FADE_MIN_VAL) leds[index].g+=amount;
  if (leds[index].b > (float)(color.b) * minValPercentage && leds[index].b >= MOVING_FADE_MIN_VAL) leds[index].b+=amount;
}

bool isColorUp(CRGB color1, CRGB color2)
{
  int c1 = color1.r + color1.g + color1.b;
  int c2 = color2.r + color2.g + color2.b;
  return c1 > c2;
}

void fadeColorUpdate()
{
 if (fadeColorTime == 0) fadeColorTime = millis();
 if (millis() - fadeColorTime >= FADE_COLOR_UPDATE_TIME)
 {
  fadeColorTime = millis();
  fadecolorState ++;
  if (fadecolorState >= STATIC_COLOR_COUNT)
  {
    fadecolorState = 0;
  }
  baseColor = static_colors[fadecolorState];
 }
}

void shimmer()
{
  for (int i = 0; i < LED_NUM; i++)
  {
    if (isColorUp(leds[i], shimmerTo[i]))
    {
      updateShimmerLight(i, -1);
      if (isArriving(leds[i], shimmerTo[i]))
      {
        leds[i] = shimmerTo[i];
        updateShimmerTarget(i);
      }
    }
    else
    {
      updateShimmerLight(i, 1);
      if (isArriving(leds[i], shimmerTo[i]))
      {
        leds[i] = shimmerTo[i];
        updateShimmerTarget(i);
      }
    }
  }
  FastLED.show();
}

void updateShimmerLight(int i, int multiplier)
{
  leds[i].r = updateShimmerSingleChannel(leds[i].r, shimmerTo[i].r, shimmerMove[i].r, multiplier);
  leds[i].g = updateShimmerSingleChannel(leds[i].g, shimmerTo[i].g, shimmerMove[i].g, multiplier);
  leds[i].b = updateShimmerSingleChannel(leds[i].b, shimmerTo[i].b, shimmerMove[i].b, multiplier);
}

int updateShimmerSingleChannel(int ledChannel, int shimmerToChannel, int shimmerMoveChannel, int multiplier)
{
  if (ledChannel != shimmerToChannel)
  {
    if (multiplier == -1)
    {
      if (ledChannel - shimmerMoveChannel <= shimmerToChannel) 
        ledChannel = shimmerToChannel;
      else
        ledChannel -= shimmerMoveChannel;
    }
    else
    {
      if (ledChannel + shimmerMoveChannel >= shimmerToChannel) 
        ledChannel = shimmerToChannel;
      else
        ledChannel += shimmerMoveChannel;
    }
  }
  return ledChannel;
}

bool isArriving(CRGB c1, CRGB c2)
{
  int val1 = c1.r + c1.g + c1.b;
  int val2 = c2.r + c2.g + c2.b;
  
  return (val2 - shimmer_threshhold <= val1 && val1 <= val2 + shimmer_threshhold);
}

CRGB getBaseColor(int index)
{
  if (colorState == STATIC_COLOR_COUNT+1)
  {
    return getChristmasColor(index);
  }
  return baseColor;
}

void checkButtonPress()
{
  unsigned long timePassed = millis() - startHold;
  if (timePassed < 500)
  {
    buttonPress();
  }
  else if (timePassed >= 500 && timePassed < 2000)
  {
    buttonHold();
  }
  else if (timePassed >= 2000)
  {
    setStrobe();
  }
  startHold = 0;
}

void setStrobe()
{
  isStrobe = !isStrobe;
  if (isStrobe)
  {
    setStrip();  
  }
}

void doStrobe()
{
  strobeTime += millis() - strobeTime;
  if (strobeTime >= STROBE_TIME)
  {

  }
}

//single quick button press
void buttonPress()
{
  colorState++;
    if (colorState > STATIC_COLOR_COUNT+1)
    {
      colorState = 0;
    }
    else if (colorState < STATIC_COLOR_COUNT)
    {
      baseColor = static_colors[colorState];
      if (lightState == 0) setupStrip();
    }
    else if (colorState == STATIC_COLOR_COUNT+1)
    {
      CRGB color = CRGB::Red;
      for (int i = 0; i < LED_NUM; i++)
      {
        getChristmasColor(i);
      }
      setStrip();
    }
    else
    {
      fadeColorTime = millis();
    }
}

CRGB getChristmasColor(int index)
{
  int color = index / 2;
  if (color % 2 == 0)
  {
    return CRGB::Red;
  }
  else
  {
    return CRGB::Green;
  }
}

//long button hold
void buttonHold()
{
  lightState++;
    if (lightState >= LIGHT_STATE_COUNT){
      lightState = 0;
    }  
    setupStrip();
}

void setupStrip()
{
  switch(lightState)
    {
      case 0://static
        Serial.println("Setting to static");
        setStrip();
      break;
      case 1://shimmer
        Serial.println("Setting to shimmer");
        for (int i = 0; i < LED_NUM; i++)
        {
          updateShimmerTarget(i);
        }
      break;
      case 2://moving fade
        Serial.println("Setting to Moving Fade");
        movingFadeNum = 0;
        movingFadeTime = 0;
        setLEDToBaseColor(movingFadeNum);
      break;
      default:
      break;
    }
}

void updateShimmerTarget(int index)
{
  int val = random(0, shimmer_max);
  CRGB color = getBaseColor(index);
  int r = color.r;
  int g = color.g;
  int b = color.b;
  r -= val; if (r < 0) r = 0;
  g -= val; if (g < 0) g = 0;
  b -= val; if (b < 0) b = 0;
  color.r = r; color.g = g; color.b = b;
  
  shimmerTo[index] = color;
  float num = random(shimmer_min_speed, shimmer_max_speed);
  shimmerMove[index] = CRGB(num, num, num);
}

void checkButton(int button)
{
  if (button == 1 && !buttonDown)
  {
    startHold = millis();
    buttonDown = true;
  }
  else if (button == 0 && buttonDown)
  {
    buttonDown = false;
  }

}

void setStrip()
{
  for (int i = 0; i < LED_NUM; i++)
  {
    leds[i] = getBaseColor(i);
  }
  FastLED.show();
}
