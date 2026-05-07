/*
  DigitalReadSerial

  Reads a digital input on pin 2, prints the result to the Serial Monitor

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/DigitalReadSerial
*/

// digital pin 2 has a pushbutton attached to it. Give it a name:
int pushButton = 2;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_VBUSHWC);
  PIO_Configure(PIOB, PIO_OUTPUT_0, PIO_PB10A_UOTGVBOF, PIO_DEFAULT);
  PIOB->PIO_CODR = PIO_PB10A_UOTGVBOF; // =0

  Serial.begin(250000);
  SerialUSB.begin(250000);
  int t = millis();
  while(!SerialUSB.lineState()) {
    Serial.print("Waiting USB - ");
    Serial.println(millis());
  }
  t = millis() - t;
  delay(5000);
  SerialUSB.print("Init: ");
  SerialUSB.println(t);
  Serial.print("Init: ");
  Serial.println(t);
  // make the pushbutton's pin an input:
  pinMode(pushButton, INPUT);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input pin:
  int buttonState = digitalRead(pushButton);
  // print out the state of the button:
  SerialUSB.println(millis());
  Serial.println(millis());
  delay(1000);        // delay in between reads for stability
}
