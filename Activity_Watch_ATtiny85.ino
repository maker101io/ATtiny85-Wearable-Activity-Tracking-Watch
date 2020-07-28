#include <avr/sleep.h> //Needed for sleep_mode

int sleepPin = 0; // This will put the accelerometer to sleep
int vibPin = 1; // This will trigger the vibration motor. 


int xPin = A2;
int yPin = A3;
int zPin = A1;

int vibLevel = 255; // PWM set to max

const long maxAtRest = 90;//15*60; // This is the longest that the user can be at rest for (seconds)

int thresholdHigh = 650; 
int thresholdLow = thresholdHigh - 511.5; // accel centered around 511.5 

int activityTimer = 0;

int flag = 0;

int xVal = 0;
int yVal = 0;
int zVal = 0;

int watchdog_counter = 0; 

void setup() 
{
  pinMode(vibPin, OUTPUT);
  pinMode(sleepPin, OUTPUT);

  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
  pinMode(zPin, INPUT);


  digitalWrite(sleepPin, LOW);// puts accelerometer to sleep
  delay(2000);
  
  vibMotor();
  
  watchdog_counter = 0;

  //Power down various bits of hardware to lower power usage  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); //Power down everything, wake up from WDT
  sleep_enable();
  ADCSRA &= ~(1<<ADEN); //Disable ADC, saves ~230uA
  setup_watchdog(6); //Setup watchdog to go off after 1sec
}

void loop() 
{
  sleep_mode(); //Go to sleep!

  if(activityTimer > maxAtRest) // give reminder to get active
  {
    vibMotor();
    vibMotor();
    activityTimer = 0;
    watchdog_counter = 0;
  }
  
  if(watchdog_counter > 30) 
  {
    for (int i = 0; i<5; i++) // check for activity once/second for 5 seconds.  
    {
      digitalWrite(sleepPin, HIGH);// wakes up accelerometer
      ADCSRA |= (1<<ADEN); //ADC is only on while checking accelerometer. 
      flag = flag + checkAccel();
      digitalWrite(sleepPin, LOW); 
      ADCSRA &= ~(1<<ADEN); //Disable ADC    
      if (flag>0)
      {
        activityTimer = 0;
        i = 10;
      }
      else sleep_mode();   
    }
        
    flag = 0;
    watchdog_counter = 0;
  }
}

int checkAccel()
{
    xVal = analogRead(xPin);
    yVal = analogRead(yPin);
    zVal = analogRead(zPin);

    if ( xVal < thresholdLow || xVal > thresholdHigh ||yVal < thresholdLow || yVal > thresholdHigh ||zVal < thresholdLow || zVal > thresholdHigh )
    {
        return 1;
    }
    else  return 0;
}

void vibMotor()
{
  analogWrite(vibPin, vibLevel);
  delay(500);
  analogWrite(vibPin, 0);
  delay(500);
  analogWrite(vibPin, vibLevel);
  delay(500);
  analogWrite(vibPin, 0);
}
  
//This runs each time the watch dog wakes us up from sleep
ISR(WDT_vect) {
  watchdog_counter++;
  activityTimer++;
}

void setup_watchdog(int timerPrescaler) 
{
  if (timerPrescaler > 9 ) timerPrescaler = 9; //Correct incoming amount if need be

  byte bb = timerPrescaler & 7; 
  if (timerPrescaler > 7) bb |= (1<<5); //Set the special 5th bit if necessary

  //This order of commands is important and cannot be combined
  MCUSR &= ~(1<<WDRF); //Clear the watch dog reset
  WDTCR |= (1<<WDCE) | (1<<WDE); //Set WD_change enable, set WD enable
  WDTCR = bb; //Set new watchdog timeout value
  WDTCR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int
}
