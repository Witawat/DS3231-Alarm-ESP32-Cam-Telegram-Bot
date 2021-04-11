/*
  Based on original work by
  Name:        ESP32-CAM.ino
  Created:     20/06/2020
  Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
  Description: an example that show how is possible send an image captured from a ESP32-CAM board
*/

//                                             WARNING!!!
// Make sure that you have selected ESP32 Wrover Module, or another board which has PSRAM enabled
// and Partition Schema: "Default 4MB with ffat..."

/*
    ESP32-Cam captures a photo and sends it to Telegram everytime the DS3231 alarm is tripped.

    https://garrysblog.com/2020/07/05/using-te-ds3231-real-time-clock-alarm-with-the-adafruit-rtclib-library/
    Energy is save by turning power ON to the ESP32Cam using a P-Channel Mosfet and removing components from the DS3231 board,
    namely both resistor blocks and eeprom chip, and cutting VCC pin (2). The DS3231 pulls 3uA from it's own battery when the ESP32 is off.

    Place 10K pullups on GPIO14 (SCL) and GPIO15 (SDA) for stability.

    http://asciiflow.com/

                    3.3V
                     +
                     |
                     +-----------+
                    +-+          | S
               10K  | |     | +--+
                    +++     |    |
                     |    G | +  |
          +----------+------|-+--+  AO3401 p-channel mosfet
          |                 |
          |                 | +
          |                 | +--+
          |                      |D
          |                      |
          |       +---+----------+
          |       |   |          |
    +-----+---+  +++ +++   +-----+---+
    |   SQW   |  | | | |10K| ESP32CAM|
    |         |  | | | |   |         |
    |  DS3231 |  +++ +++   |         |
    |         |   |   |    |         |
    |     SDA +---+--------+IO14     |
    |         |       |    |         |
    |     SCL +-------+----+IO15     |
    |         |            |         |
    +-----+---+            +----+----+
          |                     |
          +-----------+---------+
                      |
                      +
                   GROUND


    Everything ESP32-CAM:
    https://RandomNerdTutorials.com/telegram-esp32-cam-photo-arduino/
    https://github.com/robotzero1

    lowpower with DS3231:
    https://thecavepearlproject.org/read-the-blog/ (ds3231 mod pictures)
    https://sites.google.com/site/wayneholder/low-power-techniques-for-arduino

    Libraries:
    https://github.com/cotestatnt/asynctelegram
    https://github.com/adafruit/RTClib

    IMPORTANT!!!
    - Use a power supply capable of delivering close to 500mA current. Choose voltage regulator carefully
*/

#include <RTClib.h>
#include <Wire.h>
RTC_DS3231 rtc;
#define I2C_SDA 15
#define I2C_SCL 14
#define DS3231_I2C_ADDRESS 0x68
#define DS3231_CONTROL_REG 0x0E

#include "esp_camera.h"
#include "soc/soc.h"           // Brownout error fix
#include "soc/rtc_cntl_reg.h"  // Brownout error fix
// Define where store images (on board SD card reader or internal flash memory)
#include <FFat.h>              // Use internal flash memory
fs::FS &filesystem = FFat;     // Is necessary select the proper partition scheme (ex. "Default 4MB with ffta..")
// You only need to format FFat when error on mount (don't work with MMC SD card)
#define FORMAT_FS_IF_FAILED true
#define FILENAME_SIZE 20
#define KEEP_IMAGE false
#include <WiFi.h>
#include <AsyncTelegram.h>
AsyncTelegram myBot;

const char* ssid = "00000";             // REPLACE mySSID WITH YOUR WIFI SSID
const char* pass = "00000";          // REPLACE myPassword YOUR WIFI PASSWORD, IF ANY
const char* token = "00000";     // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN
int max_retry_count = 4;
int counter = 0;
uint32_t chatID = 00000;

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


// Struct for saving time datas (needed for time-naming the image files)
struct tm timeinfo;


// List all files saved in the selected filesystem
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
	Serial.printf("Listing directory: %s\r\n", dirname);

	File root = fs.open(dirname);
	if (!root) {
		Serial.println("- failed to open directory");
		return;
	}
	if (!root.isDirectory()) {
		Serial.println(" - not a directory");
		return;
	}

	File file = root.openNextFile();
	while (file) {
		if (file.isDirectory()) {
			Serial.printf("  DIR : %s", file.name());
			if (levels)
				listDir(fs, file.name(), levels - 1);
		} else
			Serial.printf("  FILE: %s\tSIZE: %d\n", file.name(), file.size());

		file = root.openNextFile();
	}
}

String takePicture(fs::FS &fs) {

	// Set filename with current timestamp "YYYYMMDD_HHMMSS.jpg"
	char pictureName[FILENAME_SIZE];
	getLocalTime(&timeinfo);
	snprintf(pictureName, FILENAME_SIZE, "%02d%02d%02d_%02d%02d%02d.jpg", timeinfo.tm_year + 1900,
	         timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

	// Path where new picture will be saved
	String path = "/";
	path += String(pictureName);

	File file = fs.open(path.c_str(), FILE_WRITE);
	if (!file) {
		Serial.println("Failed to open file in writing mode");
		return "";
	}

	// Take Picture with Camera

	camera_fb_t * fb = NULL;
	delay(2000); // 2000 ms delay helps sensor stabilize & prevented poor colors and overexposed photos
	fb = esp_camera_fb_get();

	
	int retries = 0; // if capture fails, retry a number of times before rebooting
	if (!fb)
		while (1) {
			Serial.println("Not having image yet, waiting a bit");
			fb = esp_camera_fb_get();
			if (fb) break;
			retries++;
			if (retries > max_retry_count)ESP.restart();
			delay(500);
		}

	// Save picture to memory
	uint64_t freeBytes =  FFat.freeBytes();

	if (freeBytes > fb->len ) {
		file.write(fb->buf, fb->len); // payload (image), payload length
		Serial.printf("Saved file to path: %s\n", path.c_str());
		file.close();
	} else
		Serial.println("Not enough space avalaible");

	esp_camera_fb_return(fb);
	return path;
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
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

	Serial.begin(115200);
	Serial.setDebugOutput(true);
	Serial.println();
	// FFat.format(); // Format internal flash memory if there is something wrong with photo storage

	Wire.begin(I2C_SDA, I2C_SCL);
	delay(100);

	// initializing the rtc
	if (!rtc.begin()) {
		Serial.println("Couldn't find RTC!");
		Serial.flush();
		abort();
	}

	// Only needed if you cut the Vcc pin supplying power to the DS3231 chip to run clock from coincell
	enableRTCAlarmsonBackupBattery();

	if (rtc.lostPower()) {
		// this will adjust to the date and time at compilation
		rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	}

	// January 21, 2014 at 3am you would call:
	//  rtc.adjust(DateTime(2021, 3, 31, 12, 11, 0));

	//we don't need the 32K Pin, so disable it
	rtc.disable32K();

	// stop oscillating signals at SQW Pin
	// otherwise setAlarm1 will fail
	rtc.writeSqwPinMode(DS3231_OFF);

	// turn off alarm 2 (in case it isn't off already)
	// again, this isn't done at reboot, so a previously set alarm could easily go overlooked
	rtc.disableAlarm(2);

	// cameraSetup
	camera_config_t config;
	config.ledc_channel = LEDC_CHANNEL_0;
	config.ledc_timer = LEDC_TIMER_0;
	config.pin_d0 = Y2_GPIO_NUM;
	config.pin_d1 = Y3_GPIO_NUM;
	config.pin_d2 = Y4_GPIO_NUM;
	config.pin_d3 = Y5_GPIO_NUM;
	config.pin_d4 = Y6_GPIO_NUM;
	config.pin_d5 = Y7_GPIO_NUM;
	config.pin_d6 = Y8_GPIO_NUM;
	config.pin_d7 = Y9_GPIO_NUM;
	config.pin_xclk = XCLK_GPIO_NUM;
	config.pin_pclk = PCLK_GPIO_NUM;
	config.pin_vsync = VSYNC_GPIO_NUM;
	config.pin_href = HREF_GPIO_NUM;
	config.pin_sscb_sda = SIOD_GPIO_NUM;
	config.pin_sscb_scl = SIOC_GPIO_NUM;
	config.pin_pwdn = PWDN_GPIO_NUM;
	config.pin_reset = RESET_GPIO_NUM;
	config.xclk_freq_hz = 20000000;
	config.pixel_format = PIXFORMAT_JPEG;

	//init with high specs to pre-allocate larger buffers
	if (psramFound()) {
		config.frame_size = FRAMESIZE_UXGA;
		//config.jpeg_quality = 10;  //0-63 lower number means higher quality
		config.jpeg_quality = 10;  //0-63 lower number means higher quality
		config.fb_count = 2;
	} else {
		config.frame_size = FRAMESIZE_SVGA;
		config.jpeg_quality = 12;  //0-63 lower number means higher quality
		config.fb_count = 1;
	}

	// camera init
	esp_err_t err = esp_camera_init(&config);
	if (err != ESP_OK) {
		Serial.printf("Camera init failed with error 0x%x", err);
		delay(1000);
		ESP.restart();
	}

	sensor_t * s = esp_camera_sensor_get();

	s->set_framesize(s, FRAMESIZE_XGA);  // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA

	//  s->set_brightness(s, );     // -2 to 2
	//  s->set_contrast(s, 0);       // -2 to 2
	//  s->set_saturation(s, 0);     // -2 to 2
	//  s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
	s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
	s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
	s->set_wb_mode(s, 2);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
	//  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
	//  s->set_aec2(s, 0);           // 0 = disable , 1 = enable
	//  s->set_ae_level(s, 0);       // -2 to 2
	//  s->set_aec_value(s, 300);    // 0 to 1200
	//  s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
	//  s->set_agc_gain(s, 0);       // 0 to 30
	//  s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
	//  s->set_bpc(s, 0);            // 0 = disable , 1 = enable
	//  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
	//  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
	//  s->set_lenc(s, 1);           // 0 = disable , 1 = enable
	//  s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
	//  s->set_vflip(s, 0);          // 0 = disable , 1 = enable
	//  s->set_dcw(s, 1);            // 0 = disable , 1 = enable
	//  s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

	// Init WiFi connections
	WiFi.begin(ssid, pass);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		counter++;
		if (counter >= 60) { //after 30 seconds timeout - reset board (on unsucessful connection)
			ESP.restart();
		}
	}
	Serial.print("\nWiFi connected: ");
	Serial.print(WiFi.localIP());

	// Init filesystem (format if necessary)
	if (!FFat.begin(FORMAT_FS_IF_FAILED))
		Serial.println("\nFS Mount Failed.\nFilesystem will be formatted, please wait.");
	Serial.printf("\nTotal space: %10lu\n", FFat.totalBytes());
	Serial.printf("Free space: %10lu\n", FFat.freeBytes());

	listDir(filesystem, "/", 0);

	// Set the Telegram bot properies
	myBot.setUpdateTime(1000);
	myBot.setTelegramToken(token);

	// Check if all things are ok
	Serial.print("\nTest Telegram connection... ");
	myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

	// Init and get the system time
	configTime(3600, 3600, "pool.ntp.org");
	getLocalTime(&timeinfo);
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

	// Send RSSI
	TBMessage msg;
	msg.chatId = chatID;
	long rssi = 0;
	long averageRSSI = 0;
	for (int i = 0; i < 5; i++) {
		rssi += WiFi.RSSI();
		delay(20);
	}
	averageRSSI = rssi / 5;
	String  message_rssi = String(averageRSSI);
	myBot.sendMessage(msg, message_rssi);

	delay(1000);

	// Take picture and send it
	String myFile = takePicture(filesystem);
	if (myFile != "") {
		if (!myBot.sendPhotoByFile( chatID, myFile, filesystem))
			Serial.println("Photo send failed");
		//If you don't need to keep image in memory, delete it
		if (KEEP_IMAGE == false) {
			filesystem.remove("/" + myFile);
		}
	}

	// Set alarm to 10min from present time
	// If present hour is 21h, 22h, 23h, 0h, 1h, 2h,3h, 4h or 5h, alarm is set 1 hour away
	DateTime now = rtc.now();
	// Serial.print("rtc time: "); Serial.print(now.hour(), DEC); Serial.print(" : "); Serial.println(now.minute(), DEC);
	bool night_time = false;
	int night_hours[]= {21, 22, 23, 0, 1, 2, 3, 4, 5};
	for (int index=0; index<9; index++) {
		if (night_hours[index] == now.hour()) {
			night_time = true;
			break;
		}
	}
	if (night_time) {
		if (!rtc.setAlarm1(
		            rtc.now() + TimeSpan(0, 1, 0, 0),
		            DS3231_A1_Hour //
		        )) {
			Serial.println("Error, alarm wasn't set!");
		} else {
			Serial.println("Alarm will happen in 1 hours!");
		}
	} else {
		if (!rtc.setAlarm1(
		            rtc.now() + TimeSpan(0, 0, 10, 0),
		            DS3231_A1_Minute //
		        )) {
			Serial.println("Error, alarm wasn't set!");
		} else {
			Serial.println("Alarm will happen in 10 minutes!");
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
