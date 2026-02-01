# Women's Safety IoT Smart Locket

An ESP32-based wearable safety device that triggers SOS alerts via Gmail based on voice distress patterns, panic button activation, or abnormal heart rates.

## Features
* **Panic Button:** Immediate manual SOS trigger.
* **Voice Distress Detection:** Listens for loud/sustained screaming (MAX4466).
* **Biometric Monitoring:** Tracks heart rate (MAX30105). Triggers alert if BPM < 50 or > 120.
* **Email Alerts:** Sends distress type via SMTP to emergency contacts.

## Hardware Requirements
* **Microcontroller:** ESP32 Development Board
* **Heart Rate Sensor:** MAX30105 / MAX30102
* **Microphone:** MAX4466 (Adjustable Gain)
* **Input:** Push Button
* **Power:** LiPo Battery or USB

##  Setup & Configuration

1.  **Clone the repo:**
    ```bash
    git clone [https://github.com/prakruthisridhar/Women-Safety-IoT-Smart-Locket.git](https://github.com/prakruthisridhar/Women-Safety-IoT-Smart-Locket.git)
    ```
2.  **Important Note on Folders:**
    Arduino requires the file and folder names to match. After downloading, ensure your folder is named `women_safety_device` to match the `women_safety_device.ino` file.

3.  **Create `secrets.h`:**
    The `secrets.h` file is excluded from this repo for security. Create a file named `secrets.h` in the source folder with your WiFi and Email credentials.

4.  **Upload:**
    Connect ESP32 via USB and upload using Arduino IDE.
