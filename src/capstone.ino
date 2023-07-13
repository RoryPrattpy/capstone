// Import all the libraries
#include "SparkFun_Qwiic_Humidity_AHT20.h"
#include "oled-wing-adafruit.h"
#include "MQTT.h"
#include "env.h"
#include "blynk.h"
#include "LIS3DH.h"

// Define the pins
#define BUTTONPIN D6
#define VTEMPPIN V0
#define VHUMIDITYPIN V1
#define VWINDSPEED V2
#define VPRECIPITATION V3
#define SHOCKNOTIF "earthquake"

// Declare the functions
void getSensorData();
void callback(char *topic, byte *payload, unsigned int length);
// void connectingAnimation();
void resetDisplay();
void callback2();
void shockNotif();

SYSTEM_THREAD(ENABLED);

MQTT client("lab.thewcl.com", 1883, callback);
OledWingAdafruit display;
AHT20 AHTSensor;
LIS3DHI2C accel(Wire, 0, WKP);

// Create the variables
bool OLEDdisplayInside = true;
bool previousState = false;
bool doingAnimation = false;
int shakeSensitivity = 5000;
int shakePreviousY = 100;
int shakePreviousX = 100;

// Variables for storing environmental data
int oTemp = 0.0;
float oWindSpeed = 0.0;
int oPrecipitation = 0.0;
int oHumidity = 0.0;
int iAmount = 0;
char *oPrecipitationType = "";
bool updateData = true;
float iTemperature = 0.0;
float iHumidity = 0.0;

// Create the timers
Timer getWeatherData(30000, callback2, false);
Timer startAnimation(500, connectingAnimation, false);
Timer getSensorDataTimer(2000, getSensorData, false);

void connectingAnimation() {
  resetDisplay();
  display.print("Connecting");
  for (int i = 0; i < iAmount; i++) {
    display.print(".");
  }
  iAmount++;
  if (iAmount > 3) {
    iAmount = 0;
  }
  display.display();
}

bool stoppingFirst = true;

void setup()
{

  pinMode(BUTTONPIN, INPUT_PULLDOWN);

  display.setup();
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);

  client.connect(System.deviceID());

  startAnimation.start();
  display.print("Connecting");
  iAmount++;
  while (!client.isConnected()) {
    client.connect(System.deviceID());
  }

  client.subscribe("WeatherBox/PostData/+");

  Wire.setSpeed(CLOCK_SPEED_100KHZ);

  Wire.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN);

  getSensorDataTimer.start();
  getWeatherData.start();

  LIS3DHConfig config;
  config.setAccelMode(LIS3DH::RATE_100_HZ);
  accel.setup(config);

  while (!AHTSensor.begin())
  {
  }
}

void loop()
{
  display.loop();
  Blynk.run();

  if (stoppingFirst) {
    startAnimation.stop();
    stoppingFirst = false;
  }

  LIS3DHSample sample;
  if (accel.getSample(sample))
  {
    if (abs(sample.y - shakePreviousY) > shakeSensitivity || abs(sample.x - shakePreviousX) > shakeSensitivity)
    {
      Blynk.logEvent(SHOCKNOTIF);
    }
    shakePreviousX = sample.x;
    shakePreviousY = sample.y;
  }

  if (client.isConnected())
  {
    // startAnimation.stop();
    client.loop();

    if (updateData)
    {
      client.publish("WeatherBox/GetData", "GetData");
      updateData = false;
    }
  }
  else
  {
    // doingAnimation = true;
    client.connect(System.deviceID());
    client.subscribe("WeatherBox/PostData/+");
  }
  // Toggle between button states
  if (digitalRead(BUTTONPIN) && !previousState)
  {
    // Serial.println("Button Pressed" + String(OLEDdisplayInside));
    OLEDdisplayInside = !OLEDdisplayInside;
    previousState = true;
  }
  else if (!digitalRead(BUTTONPIN))
  {
    previousState = false;
  }
  // Switch between displaying inside temp and outside temp on the OLED
  if (!OLEDdisplayInside)
  {
    if (client.isConnected())
    {
      resetDisplay();
      display.print("Temperature: ");
      display.print(oTemp);
      display.println("F");
      display.print("Wind Speed: ");
      display.print(oWindSpeed);
      display.println("mph");
      display.print("Chance for Rain: ");
      display.print(oPrecipitation);
      display.println("%");
      display.print("Humidity: ");
      display.print(oHumidity);
      display.print("%");
      display.display();

      Blynk.virtualWrite(VTEMPPIN, iTemperature * 9 / 5 + 32.0);
      Blynk.virtualWrite(VHUMIDITYPIN, iHumidity);
      Blynk.virtualWrite(VWINDSPEED, oWindSpeed);
      Blynk.virtualWrite(VPRECIPITATION, oPrecipitation);
    }
  }
  else
  {
    resetDisplay();
    display.print("Temperature: ");
    display.print(iTemperature * 9 / 5 + 32.0);
    display.println("F");
    display.print("Humidity: ");
    display.print(iHumidity);
    display.print("%");
    display.display();

    Blynk.virtualWrite(VTEMPPIN, oTemp);
    Blynk.virtualWrite(VHUMIDITYPIN, oHumidity);
    Blynk.virtualWrite(VWINDSPEED, oWindSpeed);
    Blynk.virtualWrite(VPRECIPITATION, oPrecipitation);
  }
}

void callback2()
{
  updateData = true;
}

void getSensorData()
{
  if (AHTSensor.available() == true)
  {
    iTemperature = AHTSensor.getTemperature();
    iHumidity = AHTSensor.getHumidity();
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;

  if (String(topic).equals("WeatherBox/PostData/Temperature"))
  {
    int rf = atof(p);

    oTemp = rf;
  }
  if (String(topic).equals("WeatherBox/PostData/WindSpeed"))
  {
    float rf = atof(p);

    oWindSpeed = rf;
  }
  if (String(topic).equals("WeatherBox/PostData/Precipitation"))
  {
    int rf = atof(p);

    oPrecipitation = rf;
  }
  if (String(topic).equals("WeatherBox/PostData/Humidity"))
  {
    float rf = atof(p);

    oHumidity = rf;
  }
}

void resetDisplay()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
}