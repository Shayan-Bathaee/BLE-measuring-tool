# BLE-measuring-tool
School project where I built a measuring tool using a sensor and a PSoC6 board to determine how far an object is in centimeters. The distance can be displayed on an LCD screen or exported via Bluetooth.

To use the ultrasonic sensor, I set the trigger GPIO pin to high for 10ms to trigger the sensor to start. After doing so, the echo GPIO pin goes high, indicating that the sensor has sent out a sound wave and is waiting for a response. Once the echo pin goes low, the sensor has received the high-frequency sound wave back. By recording the amount of time the echo bit was high, we can calculate the distance the sound wave traveled to an object and back, and thus we can determine how far an object is.

For lab4.2, we were tasked with building a measuring tool that displays measurements on the LCD screen. My implementation works without busy waiting by using interrupts and timers. After the echo bit goes high, my interrupt is called and the timer starts. The timer counts clock cycles at 1Mhz, which means that each timer count represents one microsecond. Once the echo bit goes low, the interrupt is called again. This time, the timer is stopped and recorded, then some math is done to determine the distance of the object. After the distance is outputted to the screen, the trigger is sent again. 

Lab4.4 uses the same logic for calculating the distance of an object from the sensor. This time though, the recorded distance is exported via Bluetooth using the PSoC6 as a BLE peripheral advertising a read characteristic. Using a phone, tablet, or computer, you can connect to the device and use the read characteristic to read the distance measurements. The program constantly gets distance measurements from the sensor. When a Bluetooth event occurs (such as a read, connection, or disconnection), the AppCallBack interrupt is called and the Bluetooth event is serviced. If the event is a read, the PSoC6 sends the most recent distance measurement to the connected device. 

### Files
* **lab4.2**: Measuring tool that displays measurements on the LCD screen
  - **DFR0554.c**: Library I built for the LCD. Contains functions to start the display, show/blink the cursor, move the cursor, scroll, set the backlight color, clear the display, and print a string. For this program, only the start and print functions are used.
  - **DFR0554.h**: Contains function declarations for everything in DFR0554.c.
  - **main.c**: The implementation for this lab. 

* **lab4.4**: Measuring tool that sends measurements via Bluetooth


