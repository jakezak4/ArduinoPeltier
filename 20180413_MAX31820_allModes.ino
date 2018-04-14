// All Modes

#include <DallasTemperature.h>
#include <OneWire.h>
#include <LiquidCrystal.h>

#define peltierMode 1   // set which mode is to be used 
        // CenterCooling = 1   
        // CenterHeating = 2
        // AllCooling = 3
        // AllHeating = 4

#define ROT_PORT PORTC
#define ROT_PIN PINC
#define ROT_A _BV(1)
#define ROT_B _BV(2)
#define ROT_BTN _BV(3)
#define ROT_DELAY 110
#define ROT_BTN_DELAY 255
/*#define ROT_LED_PORT PORTD
#define ROT_LED_DDR DDRD
#define ROT_LED_R _BV(7)
#define ROT_LED_G _BV(6)
#define ROT_LED_B _BV(5)
#define ROT_LED_RGB (ROT_LED_R | ROT_LED_G | ROT_LED_B)
*/
#define ROT_LED_R A4
#define ROT_LED_G A5
#define ROT_LED_B 2
#define ROT_LED_ON 0
#define ROT_LED_OFF 1

const int rs = 8, en = 9, d4 = 10, d5 = 11, d6 = 12, d7 = 13;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#define URC10_MOTOR_1_DIR 4 // set motor for direction control
#define URC10_MOTOR_1_PWM 5 // set PWM for power control

#define URC10_MOTOR_2_DIR 7 // set motor for direction control
#define URC10_MOTOR_2_PWM 6 // set PWM for power control

#define COOL 0       // define direction for cooling effect 
#define HEAT 1       // define direction for heating effect 

OneWire  ds(14);  // on pin 10 (a 4.7K resistor is necessary)

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(38400);//);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  Serial.println("12\t30\t\n"); //set temp range before dynamic 
  Serial.print(" ");
  
  lcd.begin(20, 4);
  lcd.cursor();
  
  pinMode(14, OUTPUT);

  // setting Peltier modes
  int centerDirection; 
  int cornerDirection; 
 
  if (peltierMode == 1){
    centerDirection = COOL;
    cornerDirection = HEAT;
    } 
  else if (peltierMode == 2){
    centerDirection = HEAT;
    cornerDirection = COOL;
    } 
  else if (peltierMode == 3){
    centerDirection = COOL;
    cornerDirection = COOL;
    }
  else if (peltierMode == 4){
    centerDirection = HEAT;
    cornerDirection = HEAT;
    } 
  else {
    lcd.setCursor(0, 0);
    lcd.print("ERROR1 ");
  }
  
  pinMode(URC10_MOTOR_1_DIR, OUTPUT);
  digitalWrite(URC10_MOTOR_1_DIR, centerDirection);
  pinMode(URC10_MOTOR_1_PWM, OUTPUT);  

  pinMode(URC10_MOTOR_2_DIR, OUTPUT);
  digitalWrite(URC10_MOTOR_2_DIR, cornerDirection);
  pinMode(URC10_MOTOR_2_PWM, OUTPUT);  
  
  // rotory encoder 
  OCR2A = ROT_DELAY;
  TCCR2B = 0;
  TCCR2A = _BV(WGM20);
  TIFR2 |= _BV(OCF2A);
  TIMSK2 |= _BV(OCIE2A);

  ROT_PORT |= ROT_A | ROT_B;
  PCMSK1 |= ROT_A | ROT_B | ROT_BTN;
  PCIFR |= _BV(PCIF1);
  PCICR |= _BV(PCIE1);

  pinMode(ROT_LED_R, OUTPUT);
  pinMode(ROT_LED_G, OUTPUT);
  pinMode(ROT_LED_B, OUTPUT);
  digitalWrite(ROT_LED_G, ROT_LED_OFF);
}

byte clamp255(int v) {
  if (v < 0)
    return 0;
  else if (v > 255)
    return 255;
  else
    return v;
}

byte cornerPower = 0;
byte centerPower = 0;
volatile int _b = 0;
volatile byte _lastRot = 0;
volatile char _sign = 0;

bool settingCenter() {
  return _b % 2;
}

ISR(TIMER2_COMPA_vect) {
  TCCR2B = 0;

  if (_sign == 0)
    _b++;
  else {
    if (settingCenter())
      centerPower = clamp255(centerPower + _sign);
    else
      cornerPower = clamp255(cornerPower + _sign);
  }
}

ISR(PCINT1_vect) {
  byte _rot = ROT_PIN;
  if ((_lastRot ^ _rot) & ROT_A && (_rot & ROT_B) == 0) {
    if (_rot & ROT_A) {
      TCCR2B = 0;
    } else {
      _sign = -1;
      setDelay(ROT_DELAY);
    }
  }
  if ((_lastRot ^ _rot) & ROT_B && (_rot & ROT_A) == 0) {
    if (_rot & ROT_B) {
      TCCR2B = 0;
    } else {
      _sign = 1;
      setDelay(ROT_DELAY);
    }
  }
  if ((_lastRot ^ _rot) & ROT_BTN) {
    if (_rot & ROT_BTN) {
      TCCR2B = 0;
    } else {
      _sign = 0;
      setDelay(ROT_BTN_DELAY);
    }
  }

  _lastRot = _rot;
}

void setDelay(byte dly) {
  OCR2A = dly;
  TCNT2 = 1;
  TCCR2B = _BV(CS22) | _BV(CS20) | _BV(WGM22);
}

int nTempSensor = 0;

void loop(void) {//return;//Serial.write('M');delay(1000);return;
  //set LCD and nob color based mode selection 
  if (peltierMode == 1) {
    // center cold
    lcd.setCursor(0, 0); 
    lcd.print("HOT ");
    lcd.print(cornerPower);
    lcd.print("  ");
    lcd.setCursor(8, 0);
    lcd.print("COLD ");
    lcd.print(centerPower);
    lcd.print(" ");

    lcd.setCursor(0, 1);
    lcd.print("CENTER COOL ");
    lcd.print("  ");

    if (settingCenter()) {
      digitalWrite(ROT_LED_R, ROT_LED_OFF);
      digitalWrite(ROT_LED_B, ROT_LED_ON);
      lcd.setCursor(12, 0);
    } else {
      digitalWrite(ROT_LED_R, ROT_LED_ON);
      digitalWrite(ROT_LED_B, ROT_LED_OFF);
      lcd.setCursor(3, 0);
    }
  } else if (peltierMode == 2) { 
    // center hot 
    lcd.setCursor(0, 0);
    lcd.print("COLD ");
    lcd.print(cornerPower);
    lcd.print("  ");
    lcd.setCursor(9, 0);
    lcd.print("HOT ");
    lcd.print(centerPower);
    lcd.print(" ");

    lcd.setCursor(0, 1);
    lcd.print("CENTER HEAT ");
    lcd.print("  ");

    if (settingCenter()) {
      digitalWrite(ROT_LED_R, ROT_LED_ON);
      digitalWrite(ROT_LED_B, ROT_LED_OFF);
      lcd.setCursor(12, 0);
    } else {
      digitalWrite(ROT_LED_R, ROT_LED_OFF);
      digitalWrite(ROT_LED_B, ROT_LED_ON);
      lcd.setCursor(4, 0);
    }
  } else if (peltierMode == 3) { 
    // all cold
    lcd.setCursor(0, 0);
    lcd.print("OUTER ");
    lcd.print(cornerPower);
    lcd.print("  ");
    lcd.setCursor(10, 0);
    lcd.print("INNER ");
    lcd.print(centerPower);
    lcd.print(" ");

    lcd.setCursor(0, 1);
    lcd.print("ALL COOL ");
    lcd.print("  ");

    if (settingCenter()) {
      digitalWrite(ROT_LED_R, ROT_LED_ON);
      digitalWrite(ROT_LED_B, ROT_LED_ON);
      digitalWrite(ROT_LED_G, ROT_LED_OFF);
      lcd.setCursor(15, 0);
    } else {
      digitalWrite(ROT_LED_R, ROT_LED_ON);
      digitalWrite(ROT_LED_B, ROT_LED_ON);
      digitalWrite(ROT_LED_G, ROT_LED_ON);
      lcd.setCursor(5, 0);
    }
  } else if (peltierMode == 4){
    // all hot
    lcd.setCursor(0, 0);
    lcd.print("OUTER ");
    lcd.print(cornerPower);
    lcd.print("  ");
    lcd.setCursor(10, 0);
    lcd.print("INNER ");
    lcd.print(centerPower);
    lcd.print(" ");
    
    lcd.setCursor(0, 1);
    lcd.print("ALL HEAT ");
    lcd.print("  ");
    
    if (settingCenter()) {
      digitalWrite(ROT_LED_R, ROT_LED_OFF);
      digitalWrite(ROT_LED_B, ROT_LED_ON);
      digitalWrite(ROT_LED_G, ROT_LED_ON);
      lcd.setCursor(15, 0);
    } else {
      digitalWrite(ROT_LED_R, ROT_LED_ON);
      digitalWrite(ROT_LED_B, ROT_LED_OFF);
      digitalWrite(ROT_LED_G, ROT_LED_ON);
      lcd.setCursor(5, 0);
    }  
  } else {
    lcd.setCursor(0, 0);
    lcd.print("ERROR2 ");
  }

  //send PWM value to Peltiers 
  analogWrite(URC10_MOTOR_1_PWM, centerPower);
  analogWrite(URC10_MOTOR_2_PWM, cornerPower);
    /*
  Serial.println(ds.reset()); 
  delay(1000);
  return;
  */
  /*
  Serial.println("wut");
  delay(1000);
  return;
  */
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
  if ( !ds.search(addr)) {
    //Serial.println("No more addresses.");
    Serial.println();////
    ds.reset_search();
    delay(250);
    nTempSensor = 0;
    return;
  }
  
  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      //Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      //Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      //Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      //Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  
   for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
   }
  
  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
 
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  Serial.print(celsius);////
  Serial.print("\t");////
   //Serial.print(" Celsius, ");
  //Serial.print(fahrenheit);
  //Serial.println(" Fahrenheit");
  lcd.setCursor((nTempSensor % 3) * 7, 2 + nTempSensor / 3);
  lcd.print(celsius);
  lcd.print(" ");
   
  nTempSensor++;
}
