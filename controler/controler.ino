
void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial2.begin(115200);
}

void loop()
{
  while (0 < Serial.available())
  {
    uint8_t x = Serial.read();
    Serial1.write(x);
    Serial2.write(x);
  }
  while (0 < Serial1.available())
  {
    Serial.write(Serial1.read());
  }
  while (0 < Serial2.available())
  {
    Serial.write(Serial2.read());
  }
}
