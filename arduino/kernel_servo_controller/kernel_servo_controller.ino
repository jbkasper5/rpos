#include <Wire.h>
#include <Servo.h>

#define SLAVE_ADDR 0x08   // MUST match the 7-bit addr the Pi puts in fd->private_data
#define SERVO_PIN     9
Servo myservo;

void setup() {
  Wire.begin(SLAVE_ADDR);      // join the bus as a SLAVE at this address
  Wire.onReceive(onReceive);   // fires when the Pi WRITES to us  (your i2c_write)
  Wire.onRequest(onRequest);   // fires when the Pi READS from us (your i2c_read)
  Serial.begin(9600);        // debug console ONLY

  myservo.attach(SERVO_PIN);
  Serial.println("I2C slave ready");
}

volatile uint8_t rxbuf[32];
volatile uint8_t rxlen = 0;
volatile bool got = false;

void onReceive(int n) {                 // keep this SHORT
  uint8_t i = 0;
  while (Wire.available() && i < sizeof(rxbuf))
    rxbuf[i++] = Wire.read();           // just buffer, no printing
  rxlen = i;
  got = true;
}

// loop for printing chars
// void loop() {
//   if (got) {
//     got = false;

//     // can print bytes as hex
//     // Serial.print(rxbuf[i], HEX);
    
//     // iterate over the array and print the chars
//     for (uint8_t i = 0; i < rxlen; i++) Serial.print((char)rxbuf[i]);

//     Serial.println();
//   }
// }

// loop for reading ints
void loop() {
  if (got) {
    got = false;
    // rebuild a 32-bit little-endian int from the first 4 bytes
    uint32_t value = (uint32_t)rxbuf[0]
                   | ((uint32_t)rxbuf[1] << 8)
                   | ((uint32_t)rxbuf[2] << 16)
                   | ((uint32_t)rxbuf[3] << 24);
    Serial.println(value);        // prints the decimal value

    int angle = map(value, 0, 1023, 0, 180);   // scale to the servo's range
    myservo.write(angle);
    delay(15); 
  }
}

void onRequest() {                   // Pi is reading from us
  Wire.write(0x42);                  // supply the byte(s) it clocks out
}