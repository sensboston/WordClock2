# WordClock2
Different implementation of "word clock" (Russian language based)

This "word clock" idea came from [Vladislav Popov project](https://github.com/Vladislav-Popov/WordClock) but instead of metal plate, [IKEA's GLASSVIK door](https://www.ikea.com/us/en/p/glassvik-glass-door-black-smoked-glass-90291660/) was used as clock case.
WordClock2 also requires WiFi connection and 5V power supply, real time clock shield is optional, same as extended demo: comment defines at the beginning of the sketch.

I've used [ESP8266 WeMos D1 mini board](https://www.aliexpress.com/item/32651747570.html?spm=a2g0s.9042311.0.0.1d7b4c4dTxITbs) but any ESP8266 or ESP32 (i.e. WiFi enabled) board can be used.

After uploading sketch, you need to connect Word Clock 2 to internet. Using your smartphone or PC, connect to the new WiFi access point called **WordClock**, and configure WiFi settings. Please note: after updating using OTA, you need to reconfigure WiFi again - OTA update resets all stored credentials and settings.

[![Word clock 2 demo](https://i9.ytimg.com/vi/m1BdXO-6UY0/mq1.jpg?sqp=CIjB840G&rs=AOn4CLBO_dV8YC4JmMbuIN-XGv1568BeBw)](https://youtu.be/m1BdXO-6UY0)

Web UI schreenshots:

<img width="280px" src="https://user-images.githubusercontent.com/1036158/146594212-b02bc90b-3709-43fc-b62f-4a554606ced1.jpg"/>  <img width="280px" src="https://user-images.githubusercontent.com/1036158/146594893-2bb4205a-cee3-4b5c-af30-b7004d0172ba.jpg"/> 
<img width="280px" src="https://user-images.githubusercontent.com/1036158/146594905-312b56ed-955e-4657-8028-43a7940d9119.jpg"/>  <img width="280px" src="https://user-images.githubusercontent.com/1036158/146594915-2342ba84-05cb-47ce-aa70-fac2d4f1885e.jpg"/>
<img width="280px" src="https://user-images.githubusercontent.com/1036158/146594921-153549d2-555d-443a-b96b-251f94f4b255.jpg"/>
