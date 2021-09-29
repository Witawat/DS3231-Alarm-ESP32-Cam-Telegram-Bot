# DS3231-Alarm-ESP32-Cam-Telegram-Bot
Use DS3231 RTC and ESP-32 Camera to send a photo to a Telegram bot when alarm is tripped

**Power ON > send photo and message > alarm is set for the next power ON > alarm is enabled > Power turns OFF**

https://youtu.be/Bjpfm8ItYG8 (timelapse done with virtualdub using photos exported from Telegram Desktop)

This project utilizes the following Arduino libraries:

https://github.com/cotestatnt/AsyncTelegram
https://github.com/cotestatnt/AsyncTelegram/blob/master/REFERENCE.md

https://github.com/adafruit/RTClib

(ESP32-CAMasyncTelegramSleepTimerRSSI.ino uses esp32 deep sleep only, no ds3231)

https://asciiflow.com/ http://www.jave.de/

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

       
       
Breadboard version powered by 4xAA thought HT7333 LDO
![IMG_20210320_174229](https://user-images.githubusercontent.com/36670323/111881687-9fe51180-89a9-11eb-8e13-eee7080ce82a.jpg)
![IMG_20210320_174409](https://user-images.githubusercontent.com/36670323/111881694-a70c1f80-89a9-11eb-85df-20c2ff99e3ca.jpg)



Perfboard prototype with 1400mAh smartphone battery through HT7333 LDO
![IMG_20210320_174740](https://user-images.githubusercontent.com/36670323/111881698-aa071000-89a9-11eb-863c-77b2024d56d9.jpg)
![IMG_20210313_174228_325](https://user-images.githubusercontent.com/36670323/111881685-9e1b4e00-89a9-11eb-9b39-cc6922773858.jpg)
![IMG_20210320_174339](https://user-images.githubusercontent.com/36670323/111881689-a2476b80-89a9-11eb-8b6d-d4bba25b7ac0.jpg)



Perfboard prototype with DS3231 chip and 3V coin cell under camera
![IMG_20210401_190908](https://user-images.githubusercontent.com/36670323/113352073-b6895200-9333-11eb-9c54-8ee7d324806d.jpg)



Example photo in UXGA resolution, whie balance set to Cloudy
![025](https://user-images.githubusercontent.com/36670323/111881804-2568c180-89aa-11eb-9ecb-3f1ee945ff0e.jpg)
