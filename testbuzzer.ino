
const int buzzerPin = 13;
void setup() {
  // put your setup code here, to run once:
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(buzzerPin, HIGH);       // sets the digital pin 13 on
  delay(1000);                  // waits for a second
  digitalWrite(buzzerPin, LOW);        // sets the digital pin 13 off
  delay(1000);                  // waits for a second
}