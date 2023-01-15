#include <Wire.h>

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);
  while (!Serial);
//  Serial.println("Hello");
}

uint8_t bytes;
uint8_t state;
void loop() {
  // put your main code here, to run repeatedly:
  state = 0;
  bytes = Wire.requestFrom(0x30, 1);
  while (bytes && Wire.available()) {
    state = Wire.read();
    Serial.print("SW: ");
    Serial.print(state & 1, DEC);
    Serial.print(", ENC: ");
    Serial.println((int8_t)(state & ~1), DEC);
    bytes--;
  }
  
  delay(10);
}
