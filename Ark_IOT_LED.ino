/********************************************************************************
    Proof of Concept Vending Maching Project
	This projects illustrates a vending machine that accepts Ark Cryptocurrency (or a forked bridgechain) for payment.

    Ark_IOT_LED.ino
    2019 @phillipjacobsen

    Program Features:
    This program has been tested with ESP32 Adafruit Huzzah however it should also work with ESP8266 modules with minor changes to hardware connections and wifi libraries.
    This sketch uses the ARK Cpp-Client API to interact with an Ark V2 Devnet node or Custom Ark Bridgechain
    Ark Cpp Client available from Ark Ecosystem <info@ark.io>
    Ark API documentation:  https://docs.ark.io/sdk/clients/usage.html

    Electronic Hardware Peripherals:
    Adafruit NeoPixels

  Description of the current program flow.  status/debug info is also regularly sent to serial terminal
  1. configure peripherals
  -setup wifi and display connection status and IP address on TFT Screen
  -setup time sync with NTP server and display current time
  -check to see if Ark node is synced and display status
  2. search through all received transactions on wallet. Wallet address is displayed
    -"searching wallet + page#" will be displayed. text will toggle between red/white every received transaction
  3. # of transactions in wallet will be displayed
  4. User Interface with 3 buttons are displayed(only 1 button currently functions("M&M").
  5. When user selects M&M's a QRcode is displayed along with a timeout displayed.
  4. QR code includes price, address and Vendor field = "ArkVend_(random number)" when scanned by Ark mobile wallet.
  5. wallet waits for transaction with vendor field to be received.
  6. If payment is received then it will indicated success and display in green text "Payment: ArkVend_(random_number)"
    if incorrect payment is received then received transaction will be ignored.
  7. Back to Step 4

********************************************************************************/
// conditional assembly
//
#define NYBBLE   //this configures system for my custom bridgechain. If undefined then system will be configured for Ark Devnet.

#define ARDUINOJSON_USE_LONG_LONG 1     //required for compatiblity between Telegram Library and ArkCrypto library.

//#define RUN_TELEGRAM_CORE0  //define this to run Telegram interface as a task on Core0. Normally the Arduino application runs on Core1 and the WiFi stack on Core1.

/********************************************************************************
               Electronic Hardware Requirements and Pin Connections
    ESP32 Adafruit Huzzah
      Source: https://www.adafruit.com/product/3405
      https://randomnerdtutorials.com/esp32-pinout-reference-gpios/

    TFT FeatherWing 2.4" 320x240 Touchscreen
      Source: https://www.adafruit.com/product/3315
      Touchscreen is designed to plug direction into ESP32 Huzzah module
    		TFT_CS 	-> pin #15
    		TFT_DC 	-> pin #33
    		RT 		-> pin #32
    		SD		-> pin #14
    		SCK		-> SCK
    		MISO	-> MISO
    		MOSI	-> MOSI


   LED -> pin 13  (LED integrated on Huzzah module)

  //NEOPIXELS NOT CONNECTED YET.
		NEOPIXEL-> pin 12
		VCC -> 3.3V
		GND

********************************************************************************/


/********************************************************************************
                              Library Requirements
********************************************************************************/
const int ledPin = 13;    //LED integrated in Adafruit HUZZAH32
int ledStatus = 0;




/********************************************************************************

  // I NEED TO UPDATE COMMENTS FOR ESP32 module. These comments are for ESP8266

    Makuna NeoPixel Library - optimized for ESP8266
      Available through Arduino Library Manager however development is done using lastest Master Branch on Github
      https://github.com/Makuna/NeoPixelBus/

      This library is optimized to use the DMA on the ESP8266 for minimal cup usage. The standard Adafruit library has the potential to interfere with the
      WiFi processing done by the low level SDK
      NeoPixelBus<FEATURE, METHOD> strip(pixelCount, pixelPin);
       NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(16);
      On the ESP8266 the Neo800KbpsMethod method will use this underlying method: NeoEsp8266Dma800KbpsMethod
      The NeoEsp8266Dma800KbpsMethod is the underlying method that gets used if you use Neo800KbpsMethod on Esp8266 platforms. There should be no need to use it directly.
      The NeoEsp8266Dma800KbpsMethod only supports the RDX0/GPIO3 pin. The Pin argument is omitted. See other esp8266 methods below if you don't have this pin available.
      This method uses very little CPU for actually sending the data to NeoPixels but it requires an extra buffer for the DMA to read from.
      Thus there is a trade off of CPU use versus memory use. The extra buffer needed is four times the size of the primary pixel buffer.
       It also requires the use of the RDX0/GPIO3 pin. The normal feature of this pin is the "Serial" receive.
      Using this DMA method will not allow you to receive serial from the primary Serial object; but it will not stop you from sending output to the terminal program of a PC
      Due to the pin overlap, there are a few things to take into consideration.
      First, when you are flashing the Esp8266, some LED types will react to the flashing and turn on.
      This is important if you have longer strips of pixels where the power use of full bright might exceed your design.
      Second, the NeoPixelBus::Begin() MUST be called after the Serial.begin().
      If they are called out of order, no pixel data will be sent as the Serial reconfigured the RDX0/GPIO3 pin to its needs.
********************************************************************************/
#include <NeoPixelBus.h>
#define PixelPin 12        //Neopixel Data Pin  connected to DMA
#define PixelCount 8       //Length of Neopixel Strand
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin); //default on ESP8266 is to use the D9(GPIO3,RXD0) pin with DMA.

#define colorSaturation 128
RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor off(0, 0, 0);
RgbColor redgreen(colorSaturation, colorSaturation, 0);
RgbColor greenblue(0, colorSaturation, colorSaturation);
RgbColor black(0);




/********************************************************************************
   Ark Client Library (version 1.2.0)
    Available through Arduino Library Manager
    https://github.com/ArkEcosystem/cpp-client
********************************************************************************/
#include <arkClient.h>
/**
    This is where you define the IP address of an Ark Node (Or bridgechain node).
    You can find more Ark Mainnet and Devnet peers here: https://github.com/ArkEcosystem/peers
    The Public API port for the V2 Ark network is '4003'
*/


#ifdef NYBBLE
const char* peer = "159.203.42.124";  //Nybble Testnet Peer
#else
const char* peer = "167.114.29.55";  //Ark Devnet Peer
#endif

int port = 4003;

//Wallet Address on bridgechain
#ifdef NYBBLE
const char* ArkAddress = "TPW83DRkPcU9KyVZfCKrXMeAKDKExMhAnE";   //NYBBLE testnet address
char QRcodeArkAddress[] = "TPW83DRkPcU9KyVZfCKrXMeAKDKExMhAnE";   //jakeIOT testnet address
const char* ArkPublicKey = "02060c5793d2d42f11c8b18018c2c1ed5d81ed0ffc0afd0fe8ef5cee2dfbd3b787";       //jakeIOT testnet public key

//Wallet Address on Ark Devnet
#else
const char* ArkAddress = "DFcWwEGwBaYCNb1wxGErGN1TJu8QdQYgCt";   //Ark Devnet address
char QRcodeArkAddress[] = "DFcWwEGwBaYCNb1wxGErGN1TJu8QdQYgCt";   //Ark Devnet address
const char* ArkPublicKey = "029b2f577bd7afd878b258d791abfb379a6ea3c9436a73a77ad6a348ad48a5c0b9";       //Ark Devnet public key
//char *QRcodeArkAddress = "DHy5z5XNKXhxztLDpT88iD2ozR7ab5Sw2w";  //compiler may place this string in a location in memory that cannot be modified
#endif

char VendorID[64];

//define the payment timeout in ms
#define PAYMENT_WAIT_TIME 90000



/**
   This is how you define a connection while speficying the API class as a 'template argument'
   You instantiate a connection by passing a IP address as a 'c_string', and the port as an 'int'.
*/
Ark::Client::Connection<Ark::Client::Api> connection(peer, port);
/**/


//I think a structure here for transaction details would be better form
//I need to do some work here to make things less hacky
//struct transactionDetails {
//   const char*  id;
//   int amount;
//   const char* senderAddress;
//   const char* vendorField;
//};

//--------------------------------------------
// these are used to store the received transation details returned from wallet search
const char*  id;            //transaction ID
int amount;                 //transactions amount
const char* senderAddress;  //transaction address of sender
const char* vendorField;    //vendor field

int lastRXpage;             //page number of the last received transaction in wallet
int searchRXpage;           //page number that is used for wallet search



/********************************************************************************
   Arduino Json Libary - works with Version5.  NOT compatible with Version6
    Available through Arduino Library Manager
    Data returned from Ark API is in JSON format.
    This libary is used to parse and deserialize the reponse
********************************************************************************/
#include <ArduinoJson.h>


/********************************************************************************
  Time Library
  required for internal clock to syncronize with NTP server.
  I need to do a bit more work in regards to Daylight savings time and the periodic sync time with the NTP service after initial syncronization
********************************************************************************/
#include "time.h"
//#include <TimeLib.h>    //https://github.com/PaulStoffregen/Time
int timezone = -6;        //set timezone:  MST
int dst = 0;              //To enable Daylight saving time set it to 3600. Otherwise, set it to 0. Not sure if this works.


unsigned long timeNow;  //variable used to hold current millis() time.
unsigned long payment_Timeout;
unsigned long timeAPIfinish;  //variable used to measure API access time
unsigned long timeAPIstart;  //variable used to measure API access time

/********************************************************************************
  WiFi Library
  If using the ESP8266 I believe you will need to #include <ESP8266WiFi.h> instead of WiFi.h
********************************************************************************/
#include <WiFi.h>
//--------------------------------------------
//This is your WiFi network parameters that you need to configure

//const char* ssid = "xxxxxxxxxx";
//const char* password = "xxxxxxxxxx";

//h
const char* ssid = "TELUS0183";
const char* password = "6z5g4hbdxi";

//w
//const char* ssid = "TELUS6428";
//const char* password = "3mmkgc9gn2";


/********************************************************************************

  Version2
  Telegram BOT
  Bot name: ARK IOT Bot2
  Bot username: arkIOT2_bot
  Use this token to access the HTTP API:
  882229581:AAFl3AKcQAxsRQa2YblEUgOprclQGezZMbA
  Keep your token secure and store it safely, it can be used by anyone to control your bot.




  For a description of the Bot API, see this page: https://core.telegram.org/bots/api
  Use this link to create/manage bot via BotFather: https://core.telegram.org/bots#6-botfather

  Telegram library written by Giacarlo Bacchio (Gianbacchio on Github)
  adapted by Brian Lough
  https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot


  after bot has been created use the following in BotFather to change the list of commands supported by your bot.
  Users will see these commands as suggestions when they type / in the chat with your bot.

  /setcommands
  then enter commands like this. all in one chat. It seems you have to add all commands at once. I am not sure how to just add a new command to the list.
  start - Show list of commands
  options -Show options keyboard
  ledon - Turn LED On
  ledoff - Turn LED Off
  status - Get LED status
  name - Get Bot name
  time - Get current Time
  balance - Get balance of wallet
  transactions - Get # of Rx transactions



********************************************************************************/

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define BOTtoken "882229581:AAFl3AKcQAxsRQa2YblEUgOprclQGezZMbA"  // your Bot Token (Get from Botfather)


WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int Bot_mtbs = 2800; //mean time between scan messages
long Bot_lasttime;   //last time messages' scan has been done
bool Start = false;




/********************************************************************************
  State Machine

********************************************************************************/
enum VendingMachineStates {DRAW_HOME, WAIT_FOR_USER, WAIT_FOR_PAY, VEND_ITEM};   //The five possible states of the Vending state machine
VendingMachineStates vmState = DRAW_HOME;   //initialize the starting state.

int ARK_mtbs = 8000; //mean time between polling Ark API for new transactions
long ARKscan_lasttime;   //last time Ark API poll has been done



/********************************************************************************
  Function prototypes
  Arduino IDE normally does its automagic here and creates all the function prototypes for you.
  We have put functions in other files so we need to manually add some prototypes as the automagic doesn't work correctly
********************************************************************************/
void setup();
int searchReceivedTransaction(const char *const address, int page, const char* &id, int &amount, const char* &senderAddress, const char* &vendorField );

//void ConfigureNeoPixels(RgbColor color);

void ArkVendingMachine();
/********************************************************************************
  End Function Prototypes
********************************************************************************/




/********************************************************************************
  MAIN LOOP
********************************************************************************/
void loop() {
  //  ArkVendingMachine();

  if (millis() > ARKscan_lasttime + ARK_mtbs)  {
    //check to see if new new transaction has been received in wallet
    searchRXpage = lastRXpage + 1;
    if ( searchReceivedTransaction(ArkAddress, searchRXpage, id, amount, senderAddress, vendorField) ) {
      //a new transaction has been received.
      lastRXpage++;
      Serial.print("Page: ");
      Serial.println(searchRXpage);
      Serial.print("Transaction id: ");
      Serial.println(id);
      Serial.print("Vendor Field: ");
      Serial.println(vendorField);

      //check to see if vendorField of new transaction matches commands
      if  (strcmp(vendorField, "led on") == 0) {
      }

      else if  (strcmp(vendorField, "led off") == 0) {
        ConfigureNeoPixels(off);
      }

      else if  (strcmp(vendorField, "color red") == 0) {
        ConfigureNeoPixels(red);
      }

      else if  (strcmp(vendorField, "color green") == 0) {

        ConfigureNeoPixels(green);
      }

      else if  (strcmp(vendorField, "color blue") == 0) {
        ConfigureNeoPixels(blue);
      }

      else {
        Serial.print("Unspecified VendorField: ");
      }


    }
    ARKscan_lasttime = millis();
  }



  /*
    if (millis() > Bot_lasttime + Bot_mtbs)  {

      timeAPIstart = millis();  //get time that API read started

      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

      timeNow = millis() - timeAPIstart;  //get current time
      Serial.print("Telegram get update time:");
      Serial.println(timeNow);

      while (numNewMessages) {
        Serial.println("got response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }

      Bot_lasttime = millis();
    }
  */

}