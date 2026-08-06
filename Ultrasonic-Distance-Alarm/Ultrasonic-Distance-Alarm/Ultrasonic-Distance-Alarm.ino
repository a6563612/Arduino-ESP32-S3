#define TRIG_PIN 13
#define ECHO_PIN 12
#define BUZZER_PIN 6

// 蜂鳴器設定
const unsigned int BUZZER_FREQUENCY = 2000;
const unsigned int PWM_RESOLUTION = 8;

// HC-SR04 有效量測範圍
const float MIN_DISTANCE_CM = 2.0F;
const float MAX_DISTANCE_CM = 400.0F;

// 距離警報門檻
const float HIGH_LEVEL_DISTANCE_CM = 30.0F;
const float MEDIUM_LEVEL_DISTANCE_CM = 60.0F;
const float LOW_LEVEL_DISTANCE_CM = 100.0F;

/*
 * 量測一次距離
 *
 * 回傳值：
 *  2～400：有效距離，單位為 cm
 *  -1：沒有收到 Echo
 */
float measureDistance()
{
    // 確保 Trig 一開始為 LOW
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    // 發出 10 微秒觸發訊號
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // 最多等待 30000 微秒
    unsigned long duration =
        pulseIn(ECHO_PIN, HIGH, 30000UL);

    if (duration == 0)
    {
        return -1.0F;
    }

    // 音速約為 0.0343 cm/us
    // 聲音會往返，因此除以 2
    float distance =
        duration * 0.0343F / 2.0F;

    return distance;
}

/*
 * 連續量測 5 次，取中位數
 *
 * 中位數可以排除偶發的錯誤距離。
 */
float getMedianDistance()
{
    const int SAMPLE_COUNT = 5;

    float values[SAMPLE_COUNT];
    int validCount = 0;

    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        float distance = measureDistance();

        if (distance >= MIN_DISTANCE_CM &&
            distance <= MAX_DISTANCE_CM)
        {
            values[validCount] = distance;
            validCount++;
        }

        // 避免前一次超音波干擾下一次量測
        delay(60);
    }

    if (validCount == 0)
    {
        return -1.0F;
    }

    // 將有效距離由小到大排序
    for (int i = 0; i < validCount - 1; i++)
    {
        for (int j = i + 1; j < validCount; j++)
        {
            if (values[j] < values[i])
            {
                float temp = values[i];
                values[i] = values[j];
                values[j] = temp;
            }
        }
    }

    // 奇數筆直接取中間值
    if (validCount % 2 == 1)
    {
        return values[validCount / 2];
    }

    // 偶數筆取中間兩筆平均
    int rightIndex = validCount / 2;
    int leftIndex = rightIndex - 1;

    return (
        values[leftIndex] +
        values[rightIndex]
    ) / 2.0F;
}

/*
 * 設定蜂鳴器音量
 *
 * 8 位元 PWM：
 * 0   = 靜音
 * 26  = 約 10%
 * 64  = 約 25%
 * 128 = 約 50%，最大音量
 */
void setBuzzerVolume(unsigned int duty)
{
    // 無源蜂鳴器通常在 50% 占空比時最響
    if (duty > 128)
    {
        duty = 128;
    }

    ledcWrite(BUZZER_PIN, duty);
}

void setup()
{
    Serial.begin(115200);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    digitalWrite(TRIG_PIN, LOW);

    /*
     * Arduino ESP32 Core 3.x：
     * 將 GPIO6 設定為 2000Hz、8位元的硬體 PWM
     */
    bool attached = ledcAttach(
        BUZZER_PIN,
        BUZZER_FREQUENCY,
        PWM_RESOLUTION
    );

    if (!attached)
    {
        Serial.println(
            "Buzzer PWM initialization failed"
        );
    }

    // 開機時先保持靜音
    setBuzzerVolume(0);

    Serial.println("Distance alarm started");
}

void loop()
{
    float distance = getMedianDistance();

    if (distance < 0.0F)
    {
        Serial.println("No echo");

        // 沒有收到超音波回波時靜音
        setBuzzerVolume(0);

        delay(100);
        return;
    }

    Serial.print("Distance: ");
    Serial.print(distance, 1);
    Serial.print(" cm, level: ");

    if (distance > LOW_LEVEL_DISTANCE_CM)
    {
        // 大於 100 cm
        Serial.println("Silent");

        setBuzzerVolume(0);
    }
    else if (distance > MEDIUM_LEVEL_DISTANCE_CM)
    {
        // 60～100 cm
        Serial.println("Low - continuous");

        setBuzzerVolume(26);
    }
    else if (distance > HIGH_LEVEL_DISTANCE_CM)
    {
        // 30～60 cm
        Serial.println("Medium - continuous");

        setBuzzerVolume(64);
    }
    else
    {
        // 30 cm 以下
        Serial.println("High - continuous");

        setBuzzerVolume(128);
    }

    delay(50);
}