# DS3231-Alarm-ESP32-Cam-Telegram-Bot
DS3231 RTC wakes up ESP-32Cam periodically and a photo is sent to a Telegram bot.

**Power ON > send photo and/or message > alarm is set for the next power ON > alarm is enabled > Power turned OFF**

https://www.youtube.com/watch?v=TPH_MbMufHI (timelapse done with virtualdub using photos exported from Telegram Desktop)

This project utilizes the following Arduino libraries:

https://github.com/cotestatnt/AsyncTelegram
https://github.com/cotestatnt/AsyncTelegram/blob/master/REFERENCE.md
https://github.com/adafruit/RTClib

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

       
![photo_2021-09-29_23-28-08](https://user-images.githubusercontent.com/36670323/136223792-ed7a5666-3771-472c-b65b-13a3b45a500c.jpg)
![photo_2021-09-29_23-28-32](https://user-images.githubusercontent.com/36670323/136223804-0d5dc23e-3cf4-48ba-a8dd-b19319dadcf9.jpg)
![photo_2021-09-29_23-29-15](https://user-images.githubusercontent.com/36670323/136223812-37b0a74b-2cf9-4ecd-9fad-3f65e6620e58.jpg)
![photo_2021-09-29_23-29-35](https://user-images.githubusercontent.com/36670323/136223817-1c0714d9-e436-4799-ac47-74c7f4622b96.jpg)
![photo_2021-09-29_23-29-50](https://user-images.githubusercontent.com/36670323/136223822-481065cd-b385-42be-a73b-43701499081c.jpg)

Another hardware version with DS3231 chip taken from the breakout board:
![photo_2021-10-12_19-05-54](https://user-images.githubusercontent.com/36670323/137007328-13057eb8-40ea-41cb-8fb2-f3b5f8bc46d1.jpg)
![photo_2021-10-12_19-06-00](https://user-images.githubusercontent.com/36670323/137007342-4ff2c10b-c9cd-4823-a012-bf4200fadbaf.jpg)
![photo_2021-10-12_19-06-04](https://user-images.githubusercontent.com/36670323/137007347-f30c5bc9-9965-4f4e-8b34-caaec48d2a28.jpg)
![photo_2021-10-12_19-06-17](https://user-images.githubusercontent.com/36670323/137007357-8d7bff62-4592-4551-889c-9d2ff87831f0.jpg)


Example photo in UXGA resolution, whie balance set to Cloudy
![025](https://user-images.githubusercontent.com/36670323/111881804-2568c180-89aa-11eb-9ecb-3f1ee945ff0e.jpg)
