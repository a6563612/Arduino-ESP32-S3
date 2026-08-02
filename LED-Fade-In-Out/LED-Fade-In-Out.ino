const int ledPin = 4;

void setup()
{
    // 將 GPIO4 設定為 PWM，頻率 5000Hz，解析度 8 bits
    ledcAttach(ledPin, 5000, 8);
}

void loop()
{
    // 漸亮
    for (int duty = 0; duty <= 255; duty++)
    {
        ledcWrite(ledPin, duty);
        delay(2);
    }

    // 漸暗
    for (int duty = 255; duty >= 0; duty--)
    {
        ledcWrite(ledPin, duty);
        delay(2);
    }
}
