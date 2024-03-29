/********************************************************************************
  This file contains functions used to configure hardware perhipherals and various libraries.
********************************************************************************/


/********************************************************************************
  This routine waits for a connection to your WiFi network according to "ssid" and "password" defined previously
********************************************************************************/
void setupWiFi() {

  WiFi.begin(ssid, password); // This starts your boards connection to WiFi.
  while (WiFi.status() != WL_CONNECTED) // This will delay your board from continuing until a WiFi connection is established.
  {
    delay(400);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());


}


/********************************************************************************
  This routine configures the ESP32 internal clock to syncronize with NTP server.

  apparently this will enable the core to periodically sync the time with the NTP server. I don't really know how often this happens
  I am not sure if daylight savings mode works correctly. Previously it seems like this was not working on ESP2866
********************************************************************************/
void setupTime() {

  configTime(timezone * 3600, dst, "pool.ntp.org", "time.nist.gov");
  // printLocalTime();
  //  delay(100);

  //wait for time to sync from servers
  while (time(nullptr) <= 100000) {
    delay(100);
  }

  time_t now = time(nullptr);   //get current time
  Serial.print("time is: ");
  Serial.println(ctime(&now));


  //  struct tm * timeinfo;
  //  time(&now);
  //  timeinfo = localtime(&now);
  //  Serial.println(timeinfo->tm_hour);
}

/****************************************/
//  https://lastminuteengineers.com/esp32-ntp-server-date-time-tutorial/
//  void printLocalTime()
//  {
//    struct tm timeinfo;
//    if (!getLocalTime(&timeinfo)) {
//      Serial.println("Failed to obtain time");
//      return;
//    }
//    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
//  }





void ConfigureNeoPixels(RgbColor color) {
  strip.SetPixelColor(0, color);
  strip.SetPixelColor(1, color);
  strip.SetPixelColor(2, color);
  strip.SetPixelColor(3, color);
  strip.SetPixelColor(4, color);
  strip.SetPixelColor(5, color);
  strip.SetPixelColor(6, color);
  strip.SetPixelColor(7, color);
  strip.Show();
  Serial.print("writing new color to neopixels:");
}




/********************************************************************************
  Configure peripherals and system
********************************************************************************/
void setup()
{
  Serial.begin(115200);         // Initialize Serial Connection for debug / display
  while ( !Serial && millis() < 20 );

  pinMode(ledPin, OUTPUT); // initialize digital ledPin as an output.

#ifdef  ESP8266    
   digitalWrite(ledPin, HIGH); // initialize pin as off    //esp8266
#endif
#ifdef  ESP32  
  digitalWrite(ledPin, LOW); // initialize pin as off    //Adafruit HUZZAH32
#endif



  //--------------------------------------------
  //configure the 2.4" TFT display and the touchscreen controller

  //  delay(3000);
  //  esp_deep_sleep_start();

  //--------------------------------------------
  //  Configure NeoPixels.
  //  NOTE! If using the ESP8266 Make sure to call strip.Begin() after you call Serial.Begin because
  //    Din pin of NeoPixel is also connected to Serial RX pin(on ESP8266) and will configure the pin for usage by the DMA interface.
  strip.Begin();
  strip.ClearTo(RgbColor(0, 0, 0)); // Initialize all pixels to 'off'

  //--------------------------------------------
  //setup WiFi connection
  setupWiFi();

  //--------------------------------------------
  //  sync local time to NTP server
  setupTime();



  //--------------------------------------------
  //  check to see if Ark Node is synced
  //  node is defined previously with "peer" and "port"
  //  returns True if node is synced to chain
  if (checkArkNodeStatus()) {
    Serial.print("\nNode is Synced: ");
  }
  else {
    Serial.print("\nNode is NOT Synced: ");
  }



  //--------------------------------------------
  //  We are now going to find the last transaction received in wallet defined previously in "ArkAddress"
  //  We will start from the oldest received transaction and keep reading transactions one at a time until we have read them all.
  //  Because we read 1 trasactions at a time the page number returned by the API tells us how many transactions there are.

  lastRXpage = getMostRecentReceivedTransaction();  //lastRXpage is equal to the page number of the last received transaction in the wallet.


  //--------------------------------------------
  //  System is now configured! Set Neo Pixels to Green
//  ConfigureNeoPixels(redgreen);

  bot.sendMessage("-228362617", "Ark IOT LED is ready for Commands", "");      //Add @RawDataBot to your group chat to find the chat id.


//  delay(1500);

  Bot_lasttime = millis();  //initialize Telegram Bot Poll timer
  
  ConfigureNeoPixels(red);
 // delay(3000);
//  esp_deep_sleep_start();

}
