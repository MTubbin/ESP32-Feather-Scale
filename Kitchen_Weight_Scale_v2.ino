// Pin Assignments: 
// OLED SDA: 23 | OLED SCL: 22
// Unit Button: 14 | Tare Button: 32

#include "HX711.h"

// OLED Display Setup
#include <Wire.h>
#include <Math.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 

// Initialize Wire (I2C) with your custom pins: SDA=23, SCL=22
TwoWire I2C_OLED = TwoWire(0); // Using bus 0
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED, -1);

// HX711 Load Cell Setup
const int LOADCELL_DOUT_PIN = 17;
const int LOADCELL_SCK_PIN = 21; // Keeping the original pins for HX711

// --- Button Definitions ---
const int UNIT_BUTTON_PIN = 14; 	 // Button for UNIT CYCLING (GPIO 14)
const int TARE_BUTTON_PIN = 32; 	 // Button for TARE/ZERO function (GPIO 32)

// --- State Variables for Debounce ---
// Debounce variables for the UNIT button
int unitButtonState = HIGH; 	 
int lastUnitButtonState = HIGH; 	 
unsigned long lastUnitDebounceTime = 0; 

// Debounce variables for the TARE button
int tareButtonState = HIGH; 	 
int lastTareButtonState = HIGH;
unsigned long lastTareDebounceTime = 0;

const unsigned long debounceDelay = 0; // Debounce time set to 10ms for fast response

// Unit Mode: 0=grams, 1=kilograms, 2=pounds
int unitMode = 0;
float current_Weight = 0.0; 

HX711 scale;

void setup() {
	Serial.begin(115200);

	// Initialize both pushbutton pins with the INTERNAL PULL-UP resistor.
	pinMode(UNIT_BUTTON_PIN, INPUT_PULLUP); 
	pinMode(TARE_BUTTON_PIN, INPUT_PULLUP);

	// Initialize custom I2C pins: SDA=23, SCL=22
	I2C_OLED.begin(23, 22);

	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
		Serial.println(F("SSD1306 allocation failed"));
		for (;;);
	}
	delay(2000); 	
	display.clearDisplay();

	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0, 35);
	display.println("Initializing Scale");
	display.display();
	delay(250);
	display.clearDisplay();

	scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

	// Note: Scale factor (411.0) and offset should be calibrated for accurate results
	scale.set_scale(411.0); 
	scale.tare(); 	 	 	 	 // Tare on startup only

	display.setCursor(0, 35);
	display.println("Taring Complete.");
	display.display();
	delay(500);
}

void loop() {
	// --- Unit Cycling Button Logic (Pin 14) ---
	int unitReading = digitalRead(UNIT_BUTTON_PIN);

	if (unitReading != lastUnitButtonState) {
    Serial.println("Unit button pushed");
		lastUnitDebounceTime = millis();
	}
	if (1) {
		if (unitReading != unitButtonState) {
			unitButtonState = unitReading;

			// Execute Unit Cycle on button press (LOW)
			if (unitButtonState == LOW) { 
				unitMode = (unitMode + 1) % 3;
				Serial.print("Unit Mode Changed: ");
				Serial.println(unitMode);
			}
		}
	}
	lastUnitButtonState = unitReading;

	// --- TARE Button Logic (Pin 32) ---
	int tareReading = digitalRead(TARE_BUTTON_PIN);

	if (tareReading != lastTareButtonState) {
    Serial.println("Tare button pushed");
		lastTareDebounceTime = millis();
	}
	if (1) {
		if (tareReading != tareButtonState) {
			tareButtonState = tareReading;

			// Execute TARE on button press (LOW)
			if (tareButtonState == LOW) {
				scale.tare(); // TARE function call
				Serial.println("--- SCALE TARE EXECUTED ---");
				
				// Provide visual feedback
				display.clearDisplay();
				display.setTextSize(2);
				display.setCursor(0, 25);
				display.println("TARE");
				display.display();
				delay(500); // Display TARE message for half a second
			}
		}
	}
	lastTareButtonState = tareReading;

	// --- Load Cell Reading and Conversion ---
	float raw_grams = scale.get_units(5); 

	// ** DEAD ZONE FIX **
	// If the absolute weight is extremely small (less than 0.1 grams), 
	// force it to 0.0 to prevent display jitter between 0 and -0.
	if (abs(raw_grams) < 0.5) {
		raw_grams = 0.0;
	}
	
	float displayValue;
	String unitString;

	switch (unitMode) {
		case 0: // Grams (g)
			displayValue = raw_grams;
			unitString = "grams (g)";
			break;

		case 1: // Kilograms (kg)
			displayValue = raw_grams / 1000.0;
			unitString = "kilograms (kg)";
			break;

		case 2: // Pounds (lb)
			displayValue = raw_grams / 453.59237; 
			unitString = "pounds (lb)";
			break;
	}
	
	// --- Display Output ---
	display.clearDisplay();
	display.setTextSize(1);
	display.setCursor(0, 0);
	display.println("Weight Scale");
	display.setTextSize(3);
	display.setCursor(0, 20);
	
	// Grams often don't need decimals, while kg/lb do
	int decimalPlaces = (unitMode == 0) ? 0 : 2; 
	
	display.print(displayValue, decimalPlaces);

	display.setTextSize(1);
	display.setCursor(0, 50);
	display.println(unitString);

	display.display();
	
	// Serial log
	Serial.print("Weight: ");
	Serial.print(displayValue, decimalPlaces);
	Serial.print(" ");
	Serial.println(unitString);
}