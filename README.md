# DS3231-Alarm-ESP32-Cam-Telegram-Bot
Use DS3231 RTC and ESP-32 Camera to send a photo to a Telegram bot when alarm is tripped


This project utilizes the following Arduino libraries:

https://github.com/cotestatnt/AsyncTelegram
https://github.com/cotestatnt/AsyncTelegram/blob/master/REFERENCE.md

https://github.com/adafruit/RTClib

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
                   

![IMG_20210320_174229](https://user-images.githubusercontent.com/36670323/111881687-9fe51180-89a9-11eb-8e13-eee7080ce82a.jpg)

![IMG_20210320_174409](https://user-images.githubusercontent.com/36670323/111881694-a70c1f80-89a9-11eb-85df-20c2ff99e3ca.jpg)

![IMG_20210320_174740](https://user-images.githubusercontent.com/36670323/111881698-aa071000-89a9-11eb-863c-77b2024d56d9.jpg)

![IMG_20210320_174339](https://user-images.githubusercontent.com/36670323/111881689-a2476b80-89a9-11eb-8b6d-d4bba25b7ac0.jpg)

![IMG_20210313_174228_325](https://user-images.githubusercontent.com/36670323/111881685-9e1b4e00-89a9-11eb-9b39-cc6922773858.jpg)

![025](https://user-images.githubusercontent.com/36670323/111881804-2568c180-89aa-11eb-9ecb-3f1ee945ff0e.jpg)
