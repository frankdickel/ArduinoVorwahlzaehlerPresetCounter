#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
#include "Adafruit_FRAM_I2C.h"

#include <arduino-timer.h>;

Adafruit_FRAM_I2C fram = Adafruit_FRAM_I2C();

#include <Encoder.h>
#define ENCODER_OPTIMIZE_INTERRUPTS

char buffer [5];

const int CLK = 2;          // Rotary Encoder zum Einstellen des Sollwertes
const int DT = 3;           // Rotary Encoder zum Einstellen des Sollwertes
const int DREH_TASTER = 4;  // Rotary Encoder Switch-Pin, Sollwert kann nur bei gedruecktem Drehknopf geaendert werden
const int RESET = 5;        // Zuruecksetzen des Ist-Wertes auf 0, Relais zieht an
const int REED = 6;         // Reedkontakt als Sensor fuer die Zaehlereignissse
const int RELAIS = 7;       // Relais ist im Bereich Istwert >= 0 und Istwert < Sollwert angezogen
const int DREH_TASTER_10 = 9;   // 10er Schritte

int countSwitches;
int targetValue;
const int SwitchPin = 0;
int val = 0;
int var = 0;
int currentState = 0;
int previousState = 0;


Encoder meinEncoder(DT, CLK);
long altePosition = -999;
long neuePosition = -999;
boolean Richtung;
int Pin_clk_Letzter;
int Pin_clk_Aktuell;
int Pin_dt_Aktuell;
int Pin_dt_Letzter;
int Counter = 0;
int positionignore = 0;

// Byte-Variable fuer Lesen/Schreiben der Ist- und Sollwerte in FRAM:
uint8_t low_value;
uint8_t high_value;
uint8_t low_value_count;
uint8_t high_value_count;

void setup()
{
    lcd.init();
    lcd.backlight();
    targetValue = 250;
    countSwitches = 0;
    //Serial.begin(38400);
    lcd.setCursor(0, 0);
    lcd.print("Soll");
    lcd.setCursor(0, 1);
    lcd.print("Ist");
    lcd.setCursor(5, 1);
    fram.begin();
    low_value_count = fram.read(0x7);
    high_value_count = fram.read(0x8);
    countSwitches = low_value_count + 256 * high_value_count;
    sprintf(buffer,"%3d",countSwitches);
    lcd.print(buffer);

    Pin_clk_Letzter = digitalRead(CLK);
    Pin_dt_Letzter = digitalRead(DT);

    low_value = fram.read(0x5);
    high_value = fram.read(0x6);

    targetValue = low_value + 256 * high_value;
    if (targetValue <= 0) targetValue = 200;

    pinMode(SwitchPin, INPUT);
    if (countSwitches >= targetValue) 
    {
        relais_aus();
        countSwitches = targetValue;
    }
    else relais_an();

    pinMode(CLK, INPUT);
    pinMode(DT, INPUT);
    pinMode(RELAIS, OUTPUT);
    pinMode(REED, INPUT_PULLUP);
    pinMode(RESET, INPUT_PULLUP);
    pinMode(DREH_TASTER, INPUT_PULLUP);
     pinMode(DREH_TASTER_10, INPUT);
    lcd.setCursor(5, 0);
    lcd.print("     ");
    lcd.setCursor(5, 0);
    sprintf(buffer,"%3d",targetValue);
    lcd.print(buffer);
}

void relais_an()
{
    digitalWrite(RELAIS, HIGH);
    lcd.setCursor(12, 1);
    lcd.print("   ");
    lcd.setCursor(12, 1);
    lcd.print("An");
}

void relais_aus()
{
    digitalWrite(RELAIS, LOW);
    lcd.setCursor(12, 1);
    lcd.print("   ");
    lcd.setCursor(12, 1);
    lcd.print("Aus");
}

void loop()
{
    long neuePosition = meinEncoder.read();  
   
    if (!(digitalRead(DREH_TASTER)) || (!digitalRead(DREH_TASTER_10)) )
    {
      
        if (neuePosition != altePosition)
        {
            if (positionignore > 5)  // ggf. Empfindlichkeit des Rotary-Encoders hier anpassen
            {
                if (altePosition < neuePosition) 
                {
                 if (!digitalRead(DREH_TASTER))
                  targetValue--; 
                   else targetValue = targetValue - 10;
                }   
                else 
                {
                  if (!digitalRead(DREH_TASTER))
                  targetValue++; 
                   else targetValue = targetValue + 10;
                }
                if (targetValue < 1) targetValue = 1;
                lcd.setCursor(5, 0);
                lcd.print("     ");
                lcd.setCursor(5, 0);
                sprintf(buffer,"%3d",targetValue);
                lcd.print(buffer);
                low_value = targetValue & 0xff; 
                high_value = targetValue >> 8;   
                fram.write(5, low_value);
                fram.write(6, high_value);
                positionignore = 0;
                //Serial.println("Dreh");
            }
            altePosition = neuePosition;
            delay(0);
            positionignore++;
        }
    }

    // Resetknpof
    if (digitalRead(RESET) == LOW)  
    {
        //elay(1000);
        countSwitches = 0;
        fram.write(7, 0);
        fram.write(8, 0);
        lcd.setCursor(5, 1);
        lcd.print("    ");
        lcd.setCursor(5, 1);
        sprintf(buffer,"%3d",countSwitches);
        lcd.print(buffer);
        relais_an();
        //Serial.println("Reset");
    }


    // Zählimpuls
    if (digitalRead(REED) == LOW)  

    {  
        currentState = 1;
    }
    else
    {
        currentState = 0;
    }
    if (currentState != previousState)
    {
        if (currentState == 1)
        {
            countSwitches = countSwitches + 1;
            //Serial.println("Reed");
            if (countSwitches >= targetValue)
            {
                relais_aus();
                //countSwitches = targetValue; // ggf. nicht mehr weiter zaehlen lassen, wenn Ist = Soll erreicht ist
            }
            lcd.setCursor(5, 1);
            lcd.print("    ");
            lcd.setCursor(5, 1);
            sprintf(buffer,"%3d",countSwitches);
            lcd.print(buffer);
            low_value_count = countSwitches & 0xff;  // Nur die unteren 8 Bit
            high_value_count = countSwitches >> 8;   // um 8 Bit nach rechts schieben
            fram.write(7, low_value_count);
            fram.write(8, high_value_count);
        }
    }
    previousState = currentState;
    delay(20);  // einfache Entprellung hier ausreichend, maximal 2 Zaehlereignisse je sec
}