/*
    http://asciiflow.com/

                 5V
                  +
                  |
                  +-----------+
                 +-+          |
            10K  | |     + +--+S
                 +++     |    |
                  |    G | +  |
     +------------+--------+--+  AO3401 P-CHANNEL MOSFET
     |                   |
     |                   | +
     |                   + +--+D
     |                        |
     |                 +------+------+
     |                 |      5V     |
     |                 |             |
     |        +---+----+3.3V         |
     |        |   |    |             |
  +----+----+  +++ +++   |             |
  |   SQW   |  | | | |10K|             |
  |         |  | | | |   |  ESP32CAM   |
  |  DS3231 |  +++ +++   |             |
  |         |   |   |    |             |
  |     SDA +---+--------+IO14         |
  |         |       |    |             |
  |     SCL +-------+----+IO15         |
  |         |            |             |
  +-----+---+            +----+--------+
      |                     |
      +-----------+---------+
                  |
                  +
               GROUND

  ESP32-Cam captures a photo and sends it to Telegram at an interval defined by DS3231 alarms.

  Based on:

  https://github.com/cotestatnt/AsyncTelegram2

  https://github.com/adafruit/RTClib
  https://garrysblog.com/2020/07/05/using-the-ds3231-real-time-clock-alarm-with-the-adafruit-rtclib-library/
*/

#include <RTClib.h>
#include <Wire.h>
RTC_DS3231 rtc;
#define I2C_SDA 15
#define I2C_SCL 14
#define DS3231_I2C_ADDRESS 0x68
#define DS3231_CONTROL_REG 0x0E

#include <WiFi.h>
#include <AsyncTelegram2.h>
#include "esp_camera.h"
#include "soc/soc.h"           // Brownout error fix
#include "soc/rtc_cntl_reg.h"  // Brownout error fix

#define USE_SSLCLIENT false
#if USE_SSLCLIENT
#include <SSLClient.h>
#include "tg_certificate.h"
WiFiClient base_client;
SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0);
#else
#include <WiFiClientSecure.h>
WiFiClientSecure client;
#endif

const char* ssid = "Reis";  // SSID WiFi network
const char* pass = "8447E21E95";  // Password  WiFi network
const char* token = "1558922737:AAEgWVlWSTQ5XZm_8yt9iJP2JHwm47usfC0";
int max_retry_count = 4;
int counter = 0;
// Check the userid with the help of bot @JsonDumpBot or @getidsbot (work also with groups)
// https://t.me/JsonDumpBot  or  https://t.me/getidsbot
int64_t userid = 789512150;

// Timezone definition to get properly time from NTP server
#define MYTZ "WET0WEST,M3.5.0/1,M10.5.0"

AsyncTelegram2 myBot(client);

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define LAMP_PIN 4

static camera_config_t camera_config = {
  .pin_pwdn = PWDN_GPIO_NUM,
  .pin_reset = RESET_GPIO_NUM,
  .pin_xclk = XCLK_GPIO_NUM,
  .pin_sscb_sda = SIOD_GPIO_NUM,
  .pin_sscb_scl = SIOC_GPIO_NUM,
  .pin_d7 = Y9_GPIO_NUM,
  .pin_d6 = Y8_GPIO_NUM,
  .pin_d5 = Y7_GPIO_NUM,
  .pin_d4 = Y6_GPIO_NUM,
  .pin_d3 = Y5_GPIO_NUM,
  .pin_d2 = Y4_GPIO_NUM,
  .pin_d1 = Y3_GPIO_NUM,
  .pin_d0 = Y2_GPIO_NUM,
  .pin_vsync = VSYNC_GPIO_NUM,
  .pin_href = HREF_GPIO_NUM,
  .pin_pclk = PCLK_GPIO_NUM,
  .xclk_freq_hz = 20000000,        //XCLK 20MHz or 10MHz
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  .pixel_format = PIXFORMAT_JPEG,  //YUV422,GRAYSCALE,RGB565,JPEG
  .frame_size = FRAMESIZE_XGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
  .jpeg_quality = 10,              //0-63 lower number means higher quality
  .fb_count = 2                    //if more than one, i2s runs in continuous mode. Use only with JPEG
};


int lampChannel = 7;           // a free PWM channel (some channels used by camera)
const int pwmfreq = 50000;     // 50K pwm frequency
const int pwmresolution = 9;   // duty cycle bit range
const int pwmMax = pow(2, pwmresolution) - 1;

// Lamp Control
void setLamp(int newVal) {
  if (newVal != -1) {
    // Apply a logarithmic function to the scale.
    int brightness = round((pow(2, (1 + (newVal * 0.02))) - 2) / 6 * pwmMax);
    ledcWrite(lampChannel, brightness);
    Serial.print("Lamp: ");
    Serial.print(newVal);
    Serial.print("%, pwm = ");
    Serial.println(brightness);
  }
}

static esp_err_t init_camera() {
  //initialize the camera
  Serial.print("Camera init... ");
  esp_err_t err = esp_camera_init(&camera_config);

  if (err != ESP_OK) {
    delay(100);  // need a delay here or the next serial o/p gets missed
    Serial.printf("\n\nCRITICAL FAILURE: Camera sensor failed to initialise.\n\n");
    Serial.printf("A full (hard, power off/on) reboot will probably be needed to recover from this.\n");
    return err;
  } else {
    Serial.println("succeeded");

    // Get a reference to the sensor
    sensor_t* s = esp_camera_sensor_get();

    // Dump camera module, warn for unsupported modules.
    switch (s->id.PID) {
      case OV9650_PID: Serial.println("WARNING: OV9650 camera module is not properly supported, will fallback to OV2640 operation"); break;
      case OV7725_PID: Serial.println("WARNING: OV7725 camera module is not properly supported, will fallback to OV2640 operation"); break;
      case OV2640_PID: Serial.println("OV2640 camera module detected"); break;
      case OV3660_PID: Serial.println("OV3660 camera module detected"); break;
      default: Serial.println("WARNING: Camera module is unknown and not properly supported, will fallback to OV2640 operation");
    }

    // Adjust sensor settings
    s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 2);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    //    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    //    s->set_vflip(s, 0);          // 0 = disable , 1 = enable

  }
  return ESP_OK;
}

size_t sendPicture(TBMessage& msg) {
  // Take Picture with Camera;
  Serial.println("Camera capture requested");

  // Take picture with Camera and send to Telegram
  //  setLamp(100);
  camera_fb_t* fb = esp_camera_fb_get();
  //  setLamp(0);
  if (!fb) {
    Serial.println("Camera capture failed");
    return 0;
  }
  size_t len = fb->len;
  if (!myBot.sendPhoto(msg, fb->buf, fb->len)) {
    len = 0;
    myBot.sendMessage(msg, "Error! Picture not sent.");
  }

  // Clear buffer
  esp_camera_fb_return(fb);
  return len;
}

void enableRTCAlarmsonBackupBattery() {
  // Enable Battery-Backed Square-Wave Enable on the DS3231 RTC module:
  // Bit 6 (Battery-Backed Square-Wave Enable) of DS3231_CONTROL_REG 0x0E, can be set to 1
  // When set to 1, it forces the wake-up alarms to occur when running the RTC from the back up battery alone.
  // [note: This bit is usually disabled (logic 0) when power is FIRST applied]
  // https://github.com/EKMallon/Utilities/blob/master/setTme/setTme.ino
  Wire.beginTransmission(DS3231_I2C_ADDRESS);      // Attention device at RTC address 0x68
  Wire.write(DS3231_CONTROL_REG);                  // move the memory pointer to CONTROL_REGister
  Wire.endTransmission();                          // complete the ‘move memory pointer’ transaction
  Wire.requestFrom(DS3231_I2C_ADDRESS, 1);         // request data from register
  byte resisterData = Wire.read();                 // byte from registerAddress
  bitSet(resisterData, 6);                         // Change bit 6 to a 1 to enable
  Wire.beginTransmission(DS3231_I2C_ADDRESS);      // Attention device at RTC address 0x68
  Wire.write(DS3231_CONTROL_REG);                  // target the CONTROL_REGister
  Wire.write(resisterData);                        // put changed byte back into CONTROL_REG
  Wire.endTransmission();
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);       // disable brownout detector

  pinMode(LAMP_PIN, OUTPUT);                       // set the lamp pin as output
  ledcSetup(lampChannel, pwmfreq, pwmresolution);  // configure LED PWM channel
  setLamp(0);                                      // set default value
  ledcAttachPin(LAMP_PIN, lampChannel);            // attach the GPIO pin to the channel

  Serial.begin(115200);
  Serial.println();

  Wire.begin(I2C_SDA, I2C_SCL);

  // initializing the rtc
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    Serial.flush();
    abort();
  }

  // Only needed if you cut the Vcc pin supplying power to the DS3231 chip to run clock from coincell
  enableRTCAlarmsonBackupBattery();

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  //we don't need the 32K Pin, so disable it
  rtc.disable32K();

  // stop oscillating signals at SQW Pin
  // otherwise setAlarm1 will fail
  rtc.writeSqwPinMode(DS3231_OFF);

  // turn off alarm 2 (in case it isn't off already)
  // again, this isn't done at reboot, so a previously set alarm could easily go overlooked
  rtc.disableAlarm(2);

  // Init the camera module (accordind the camera_config_t defined)
  init_camera();

  // Init WiFi connections
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter++;
    if (counter >= 60) {
      //ESP.restart(); //restart board if wifi not connected within 30sec
      break;
    }
  }
  Serial.print("\nWiFi connected: ");
  Serial.print(WiFi.localIP());

  // Sync time with NTP
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
#if USE_CLIENTSSL == false
  client.setCACert(telegram_cert);
#endif

  // Set the Telegram bot properies
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  // Send a welcome message to user when ready
  //  char welcome_msg[64];
  //  snprintf(welcome_msg, 64, "BOT @%s online.\nTry with /takePhoto command.", myBot.getBotName());
  //  myBot.sendTo(userid, welcome_msg);

  // Take and send photo
  TBMessage msg;
  msg.sender.id = userid;

  //  long rssi = WiFi.RSSI();
  //  myBot.sendTo(userid, String(rssi)); // Send RSSI

  Serial.println("\nSending Photo from CAM");
  if (sendPicture(msg))
    Serial.println("Picture sent successfull");
  else
    myBot.sendMessage(msg, "Error, picture not sent.");

  // Set a different power-on interval whether it's day or night time
  //
  DateTime now = rtc.now();
  Serial.print("rtc time: "); Serial.print(now.hour(), DEC); Serial.print(" : "); Serial.println(now.minute(), DEC);
  bool night_time = false;
  int night_hours[] = {20, 21, 22, 23, 0, 1, 2, 3, 4, 5};
  for (int index = 0; index < 10; index++) {
    if (night_hours[index] == now.hour()) {
      night_time = true;
      break;
    }
  }
  if (night_time) {
    if (!rtc.setAlarm1(
          rtc.now() + TimeSpan(0, 0, 1, 0),
          DS3231_A1_Hour //
        )) {
      Serial.println("Error, alarm wasn't set!");
    } else {
      Serial.println("Alarm will happen in 1 hours!");
    }
  } else {
    if (!rtc.setAlarm1(
          rtc.now() + TimeSpan(0, 0, 1, 0),
          DS3231_A1_Minute //
        )) {
      Serial.println("Error, alarm wasn't set!");
    } else {
      Serial.println("Alarm will happen in 1 minutes!");
    }
  }

  Serial.print("turn on alarm: "); Serial.println(millis()); delay(10);
  // set alarm 1, 2 flag to false (so alarm 1, 2 didn't happen so far)
  // if not done, this easily leads to problems, as both register aren't reset on reboot/recompile
  // SQW becomes HIGH, P-MOSFET opens, power to the ESP2CAM is cut
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

}

void loop() {

}
