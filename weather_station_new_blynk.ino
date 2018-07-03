#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750FVI.h>

// Точка доступа Wi-Fi
char ssid[] = "IOTIK";
char pass[] = "Terminator812";

// API key для Blynk
char auth[] = "073f88d5cffa475484641e3018533442";
IPAddress blynk_ip(139, 59, 206, 133);

// LCD windget in Blynk APP
WidgetLCD lcd(V10);

// Датчик освещенности
BH1750FVI bh1750;

// Датчик температуры/влажности и атмосферного давления
Adafruit_BME280 bme280;

// Датчик дождя
#define RAIN_PIN 14

// Датчик скорости ветра
#define WINDSPD_PIN 12

// Датчик направления ветра
#define WINDDIR_PIN A0

// Периоды для таймеров
#define BME280_UPDATE_TIME   5000
#define BH1750_UPDATE_TIME   5100
#define WINDDIR_UPDATE_TIME  5200
#define WINDSPD_UPDATE_TIME  10000
#define RAIN_UPDATE_TIME     10000

// Таймеры
BlynkTimer timer_bme280;
BlynkTimer timer_bh1750;
BlynkTimer timer_windspd;
BlynkTimer timer_winddir;
BlynkTimer timer_rain;

// Счетчик импульсов датчика дождя
float rain_rate = 0;

// Счетчик импульсов датчика скорости ветра
float wind_speed = 0;

// Параметры сенсоров для IoT сервера
#define sensorCount 7
char* sensorNames[] = {"air_temp", "air_hum", "air_press", "sun_light", "wind_spd", "wind_dir", "rain"};
float sensorValues[sensorCount];
// Номера датчиков
#define air_temp     0x00
#define air_hum      0x01
#define air_press    0x02
#define sun_light    0x03
#define wind_spd     0x04
#define wind_dir     0x05
#define rain         0x06

void setup()
{
  // Инициализация последовательного порта
  Serial.begin(115200);
  delay(512);

  // Инициализация Blynk и Wi-Fi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Blynk.begin(auth, ssid, pass, blynk_ip, 8442);

  // Инициализация входов датчиков дождя и скорости ветра
  pinMode(RAIN_PIN, INPUT_PULLUP);
  pinMode(WINDSPD_PIN, INPUT_PULLUP);

  // Инициализация прерываний на входах с импульсных датчиков
  attachInterrupt(RAIN_PIN, counterRain, FALLING);
  attachInterrupt(WINDSPD_PIN, counterWind, FALLING);

  // Однократный опрос датчиков
  readSensorBME280();
  readSensorBH1750();
  readSensorWINDSPD();
  readSensorWINDDIR();
  readSensorRAIN();

  // Вывод в терминал данных с датчиков
  printAllSensors();

  // Инициализация таймеров
  timer_bme280.setInterval(BME280_UPDATE_TIME, readSensorBME280);
  timer_bh1750.setInterval(BH1750_UPDATE_TIME, readSensorBH1750);
  timer_windspd.setInterval(WINDSPD_UPDATE_TIME, readSensorWINDSPD);
  timer_winddir.setInterval(WINDDIR_UPDATE_TIME, readSensorWINDDIR);
  timer_rain.setInterval(RAIN_UPDATE_TIME, readSensorRAIN);
}

void loop()
{
  Blynk.run();
  timer_bme280.run();
  timer_bh1750.run();
  timer_windspd.run();
  timer_winddir.run();
  timer_rain.run();
}

// Обработчик прерывания с датчика дождя
void counterRain()
{
  rain_rate = rain_rate + 1;
  Serial.println("rain");
}

// Обработчик прерывания с датчика скорости ветра
void counterWind()
{
  wind_speed = wind_speed + 1;
  Serial.println("wind");
}

// Чтение датчика BH1750
void readSensorBH1750()
{
  Wire.begin(0, 2);        // Инициализация I2C на выводах 0, 2
  Wire.setClock(10000L);   // Снижение тактовой частоты для надежности
  bh1750.begin();           // Инициализация датчика
  delay(128);
  bh1750.setMode(Continuously_High_Resolution_Mode); // Установка разрешения датчика
  sensorValues[sun_light] = bh1750.getAmbientLight();
  Blynk.virtualWrite(V3, sensorValues[sun_light]); delay(25);
  Serial.println("light");
}

// Чтение датчика BME280
void readSensorBME280()
{
  float p = 0;
  Wire.begin(4, 5);         // Инициализация I2C на выводах 4, 5
  Wire.setClock(10000L);    // Снижение тактовой частоты для надежности
  bme280.begin();           // Инициализация датчика
  delay(128);
  sensorValues[air_temp] = bme280.readTemperature();
  sensorValues[air_hum] = bme280.readHumidity();
  sensorValues[air_press] = bme280.readPressure() * 7.5006 / 1000.0;
  Blynk.virtualWrite(V0, sensorValues[air_temp]); delay(25);
  Blynk.virtualWrite(V1, sensorValues[air_hum]); delay(25);
  Blynk.virtualWrite(V2, sensorValues[air_press]); delay(25);
  Serial.println("temperature");
  Serial.println("humidity");
  Serial.println("pressure");
}

// Чтение датчика скорости ветра
void readSensorWINDSPD()
{
  sensorValues[wind_spd] = wind_speed / 10.0 * 2.4 / 3.6;
  wind_speed = 0;
  Blynk.virtualWrite(V5, sensorValues[wind_spd]); delay(25);
  Serial.println("wind speed");
}

// Чтение датчика направления ветра
void readSensorWINDDIR()
{
  double sensval = analogRead(A0) * 5.0 / 1023.0;
  double delta = 0.2;
  String wind_dir_text = "";
  if ((sensval >= (3.84 - delta)) && (sensval <= (3.84 + delta))) {
    sensorValues[wind_dir] = 0.0; // N
    wind_dir_text = "N";
  } else if ((sensval >= (1.98 - delta)) && (sensval <= (1.98 + delta))) {
    sensorValues[wind_dir] = 22.5; // NNE
    wind_dir_text = "NNE";
  } else if ((sensval >= (2.25 - delta)) && (sensval <= (2.25 + delta))) {
    sensorValues[wind_dir] = 45.0; // NE
    wind_dir_text = "NE";
  } else if ((sensval >= (0.41 - delta)) && (sensval <= (0.41 + delta))) {
    sensorValues[wind_dir] = 67.5; // ENE
    wind_dir_text = "ENE";
  } else if ((sensval >= (0.45 - delta)) && (sensval <= (0.45 + delta))) {
    sensorValues[wind_dir] = 90.0; // E
    wind_dir_text = "E";
  } else if ((sensval >= (0.32 - delta)) && (sensval <= (0.32 + delta))) {
    sensorValues[wind_dir] = 112.5; // ESE
    wind_dir_text = "ESE";
  } else if ((sensval >= (0.90 - delta)) && (sensval <= (0.90 + delta))) {
    sensorValues[wind_dir] = 135.0; // SE
    wind_dir_text = "SE";
  } else if ((sensval >= (0.62 - delta)) && (sensval <= (0.62 + delta))) {
    sensorValues[wind_dir] = 157.5; // SSE
    wind_dir_text = "SSE";
  } else if ((sensval >= (1.40 - delta)) && (sensval <= (1.40 + delta))) {
    sensorValues[wind_dir] = 180.0; // S
    wind_dir_text = "S";
  } else if ((sensval >= (1.19 - delta)) && (sensval <= (1.19 + delta))) {
    sensorValues[wind_dir] = 202.5; // SSW
    wind_dir_text = "SSW";
  } else if ((sensval >= (3.08 - delta)) && (sensval <= (3.08 + delta))) {
    sensorValues[wind_dir] = 225.0; // SW
    wind_dir_text = "SW";
  } else if ((sensval >= (2.93 - delta)) && (sensval <= (2.93 + delta))) {
    sensorValues[wind_dir] = 247.5; // WSW
    wind_dir_text = "WSW";
  } else if ((sensval >= (4.62 - delta)) && (sensval <= (4.62 + delta))) {
    sensorValues[wind_dir] = 270.0; // W
    wind_dir_text = "W";
  } else if ((sensval >= (4.04 - delta)) && (sensval <= (4.04 + delta))) {
    sensorValues[wind_dir] = 292.5; // WNW
    wind_dir_text = "WNW";
  } else if ((sensval >= (4.78 - delta)) && (sensval <= (4.78 + delta))) {
    sensorValues[wind_dir] = 315.0; // NW
    wind_dir_text = "NW";
  } else if ((sensval >= (3.43 - delta)) && (sensval <= (3.43 + delta))) {
    sensorValues[wind_dir] = 337.5; // NNW
    wind_dir_text = "NNW";
  }
  lcd.clear();
  lcd.print(0, 0, "Wind: " + String(sensorValues[wind_dir], 1) + " *");
  lcd.print(0, 1, "Wind: " + wind_dir_text);
  Serial.println("wind direction");
}

// Чтение датчика уровня осадков
void readSensorRAIN()
{
  sensorValues[rain] = rain_rate * 0.2794;
  rain_rate = 0;
  Blynk.virtualWrite(V4, sensorValues[rain]); delay(25);
  Serial.println("rain rate");
}

// Print sensors data to terminal
void printAllSensors()
{
  for (int i = 0; i < sensorCount; i++)
  {
    Serial.print(sensorNames[i]);
    Serial.print(" = ");
    Serial.println(sensorValues[i]);
  }
  Serial.println();
}

