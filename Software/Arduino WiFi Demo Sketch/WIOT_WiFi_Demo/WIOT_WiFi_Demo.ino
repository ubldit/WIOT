/************************************************
* WIOT WiFi Demo                                *
* Ubld Electronics, LLC.                        *
* http://ubld.it                                *
* Paul King & Chris Cockrum                     *
* 3/27/2015                                     *
* Released under:                               *
* Creative Commons ShareAlike 4.0 International *
************************************************/

/* Connects to Access point specified by SSID #define */
/* Password to your Access point specified by WIFI_PW #define */
/* ESP8266 gets an IP via DHCP */
/* Serves a simple web page with 2 buttons that control the user LED (D13) */

// Set these to your Access Point information
#define SSID 	"YOUR_SSID"
#define WIFI_PW "YOUR_WIFI_PASSWORD"

#include <SPI.h>
#include <Wire.h>

// This turns on/off Serial monitor info
// set to true or false
#define DEBUG false

#if DEBUG == true
  #define DEBUG_PRINT Serial.print
  #define DEBUG_PRINTLN Serial.println
#else
  #define DEBUG_PRINT
  #define DEBUG_PRINTLN
#endif

// Set esp8266 to use Serial1
#define esp8266 Serial1

// Pin defintions
// User LED (D13)
#define USER_LED 13
// wifi reset pin (D11)
#define ESP_RST 11
// wifi power pin (D12) active low
#define ESP_PWR 12

// Global Variables 
long blinkTime = 0;
boolean blink_state = 0;
int led_state = 2;

// This sets up the esp8266
void setupWifi()
{
  /* Turn on ESP-8266 Power */
  pinMode(ESP_PWR, OUTPUT);
  digitalWrite(ESP_PWR, LOW);
  
  // Reset esp8266
  pinMode(ESP_RST, OUTPUT);
  digitalWrite(ESP_RST, LOW);
  delay(50);
  digitalWrite(ESP_RST, HIGH);

  // Wait for the module to reset
  delay(500);
   
  sendData("AT\r\n",100,DEBUG); // hello
  sendData("ATE0\r\n",100,DEBUG); // echo off
  sendData("AT+CWMODE=1\r\n",100,DEBUG); // configure as connecting to an AP
  sendData("AT+CWJAP=\"" + String(SSID) + "\",\"" + String(WIFI_PW) + "\"\r\n",1000,DEBUG); // AP info
  for (int8_t i = 5; i >= 0; i--)
  {
    // Print waiting and alternate rx/tx until we are done waiting
    DEBUG_PRINT("waiting...");
    RXLED0;
    TXLED1;
    delay(500);
    RXLED1;
    TXLED0;
    delay(500);
  }
  sendData("AT+CWJAP?\r\n",100,DEBUG); // confirm connected
  sendData("AT+CIPMUX=1\r\n",100,DEBUG); // configure for multiple connections
  sendData("AT+CIPSERVER=1,80\r\n",100,DEBUG); // turn on server on port 80
  sendData("AT+CIFSR\r\n",100,DEBUG); // get ip address
  
  RXLED1;
  TXLED1;  
}

void setup()
{
  //Serial Montior
  #ifdef DEBUG
  Serial.begin(9600);
  #endif

  // Set up the UART to the Wifi Module  
  esp8266.begin(9600);
  esp8266.setTimeout(500);

  // Initializes the Wifi Module
  setupWifi();  
}

void loop()
{
  char c;
  String webpage;
  String cipSend;
  String closeCommand;
  
  // This is the 'blinky' code
  
  if(led_state == 1)
  {
    if(millis() > blinkTime)
    {
      blinkTime = millis() + 100;
      
      if(blink_state)
      {
        digitalWrite(USER_LED, HIGH);
      }
      else
      {
        digitalWrite(USER_LED, LOW);
      }      
      blink_state = !blink_state;
    }
  }
  
  if(esp8266.available()) // check if the esp is sending us a message 
  {
    RXLED0;

    if(esp8266.find("+IPD,"))
    {
     delay(100);
     
     int connectionId = esp8266.read()-48; // subtract 48 because the read() function returns 
                                           // the ASCII decimal value and 0 (the first decimal number) starts at 48
     DEBUG_PRINTLN(connectionId);

     if(esp8266.find("/"))
     {
       c = esp8266.read();
       switch(c)
       {
         case '1':
           DEBUG_PRINTLN("req led blink");
           led_state = 1;
         break;
         
         case '2':
           DEBUG_PRINTLN("req led off");
           digitalWrite(USER_LED, LOW);
           led_state = 2;
         break;
         
         default:
           DEBUG_PRINTLN("req index");
         break;
       }
     }

     // We really should wait until the browser is done sending us data
     if(esp8266.find("\r\n\r\n"))
     {
       DEBUG_PRINTLN("got end of request");
     }

     webpage = "<html><body><h1>WIOT Test</h1><h2>";
     webpage += String(millis());
     webpage += "</h2>";
     
     if(led_state == 2)
     {
       webpage += "<a href=\"/1\"><button>LED Blink</button></a>";
     }
     else
     {
       webpage += "<a href=\"/2\"><button>LED Off</button></a>";
     }
     
     webpage += "</body></html>";
     
     cipSend = "AT+CIPSEND=";
     cipSend += connectionId;
     cipSend += ",";
     cipSend += webpage.length();
     cipSend += "\r\n";
          
     sendData(cipSend,500,DEBUG);
     sendData(webpage,500,DEBUG);
    
     closeCommand = "AT+CIPCLOSE="; 
     closeCommand += connectionId; // append connection id
     closeCommand += "\r\n";
     
     sendData(closeCommand,500,DEBUG);
    }
    
    RXLED1;
  }
}

// This is used to talk to the Wifi Module
String sendData(String command, const int timeout, boolean debug)
{
    char c;
    String response = "";
 
    TXLED0;
    
    esp8266.print(command); // send the read character to the esp8266
    
    long int time = millis();
    
    while( (time+timeout) > millis())
    {
      while(esp8266.available())
      {
        // The esp has data so display its output to the serial window 
        c = esp8266.read(); // read the next character.
        response += c;
      }  
    }
    
    // This is due to some bug in the ESP8266 that causes it to go nuts and print busy messages, we reset and try again
    if(strstr(response.c_str(), "busy s...") || strstr(response.c_str(), "busy p..."))
    {
      DEBUG_PRINTLN("Busy bug, resetting wifi");
      setupWifi();
    }
    
    if(debug)
    {
      DEBUG_PRINT(response);
    }
    
    TXLED1;
    
    return response;
}