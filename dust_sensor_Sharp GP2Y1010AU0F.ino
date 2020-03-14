#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define DHTPIN 12         //pin connect DHT
#define DHTTYPE DHT22
#define N 100
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);   //Module IIC/I2C Interface

// pin numbers.
const int sharpLEDPin = 14;   // Esp8266 digital pin 7 connect to sensor LED.
const int sharpVoPin = A0;   // Esp8266 analog pin 5 connect to sensor Vo.

int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9620;

static unsigned long VoRawTotal = 0;
static int VoRawCount = 0;

// Set the typical output voltage in Volts when there is zero dust. 
static float Voc = 0.6;

// Use the typical sensitivity in units of V per 100ug/m3.
const float K = 0.5;
  
/////////////////////////////////////////////////////////////////////////////

// Helper functions to print a data value to the serial monitor.
void printValue(String text, unsigned int value, bool isLast = false) {
  Serial.print(text);
  Serial.print("=");
  Serial.print(value);
  if (!isLast) {
    Serial.print(", ");
  }
}
void printFValue(String text, float value, String units, bool isLast = false) {
  Serial.print(text);
  Serial.print("=");
  Serial.print(value);
  Serial.print(units);
  if (!isLast) {
    Serial.print(", ");
  }
}

/////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);     // Start the hardware serial port for the serial monitor.
  pinMode(sharpLEDPin, OUTPUT);   // Set LED pin for output.
  dht.begin();
  lcd.begin();
  lcd.backlight();       // เปิด backlight
}

void loop() {  
  delay(2000);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  // Turn on the dust sensor LED by setting digital pin LOW.
  digitalWrite(sharpLEDPin, LOW);

  // Wait 0.28ms before taking a reading of the output voltage as per spec.
  delayMicroseconds(samplingTime);

  // Record the output voltage. This operation takes around 100 microseconds.
  int VoRaw = analogRead(sharpVoPin);
  delayMicroseconds(deltaTime);
  
  // Turn the dust sensor LED off by setting digital pin HIGH.
  digitalWrite(sharpLEDPin, HIGH);

  // Wait for remainder of the 10ms cycle = 10000 - 280 - 100 microseconds.
  delayMicroseconds(sleepTime);
  
  // Print raw voltage value (number from 0 to 1023).
  #ifdef PRINT_RAW_DATA
  printValue("VoRaw", VoRaw, true);
  Serial.println("");
  #endif // PRINT_RAW_DATA
  
  // Use averaging if needed.
  float Vo = VoRaw;
  #ifdef USE_AVG
  VoRawTotal += VoRaw;
  VoRawCount++;
  if ( VoRawCount >= N ) {
    Vo = 1.0 * VoRawTotal / N;
    VoRawCount = 0;
    VoRawTotal = 0;
  } else {
    return;
  }
  #endif // USE_AVG

  // Compute the output voltage in Volts.
  Vo = Vo / 1024.0 * 5.0;
  printFValue("Vo", Vo*1000.0, "mV");

  // Convert to Dust Density in units of ug/m3.
  float dV = Vo - Voc;
  if ( dV < 0 ) {
    dV = 0;
    Voc = Vo;
  }
  float dustDensity = dV / K * 100.0;
  printFValue("DustDensity", dustDensity, "ug/m3", true);
  Serial.println("");

  lcd.home();
  lcd.setCursor(0, 1);
  lcd.print("Dust ");
  lcd.setCursor(5, 1);
  lcd.print(dustDensity);
  lcd.print(" ug/m3  ");

  Serial.print(" - H: ");
  Serial.print(h + String("%"));

  Serial.print(" - T: ");
  Serial.print(t + String("C"));
  
  lcd.setCursor(0, 0);          
  lcd.print("H "); 
  lcd.setCursor(2, 0);          
  lcd.print(h + String("%"));       // Show humidity value in %RH

  lcd.setCursor(8, 0);          
  lcd.print("T "); 
  lcd.setCursor(10, 0);            
  lcd.print(t + String("C"));        // Show temperature value in Celcuis   
  delay(1000);
  
}
