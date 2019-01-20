//---TankLevelSensor.ino---------------------------------------------------------
/*

Description: This sketch provides all what is needed for the TankLevelSensor.

Author:  Heiko Haegele                                          Date: 05.01.2019

Changed: Heiko Haegele                                          Date: 18.01.2019
         Major changes in MQTT and the related loop areas.

*/


//---Imports--------------------------------------------------------------------

#include <ESP8266WiFi.h>
#include <PubSubClient.h> //For MQTT

//------------------------------------------------------------------------------


//---Dependent definitions------------------------------------------------------

const char* SSID = "MyWiFi";

const char* PSK = "MyPassword";

const char* MQTT_BROKER = "192.168.178.xx";
const int   MQTT_BROKER_PORT = 1883;
const char* MQTT_TOPIC_IN  = "/MyID_ToBeChanged/in"; //Topic for incomming messages
const char* MQTT_TOPIC_OUT = "/MyID_ToBeChanged/out"; //Topic for outgoing messages

const float DISTANCE_SENSOR = 40; //[cm] Distance from Sensor to top of content when full
const float LEVEL_MAX_CONTENT = 300; //[cm] Max. possible level for full
const float LEVEL_PUMP_ON = 15; //[cm] Level, for deactivating Pump if Content is below
const float DIAMETER = 150; //[cm] My tank is a cylinder and this is the diameter

const int DEEPSLEEP_MINUTES = 1; //How long we will sleep in minutes

//------------------------------------------------------------------------------


//---Independent definitions----------------------------------------------------

//---PIN definitions---

//---D 0 (GPIO)
//Used for deepsleep wakeup RST. Therefore can not be used elsewhere

//---D 1(GPIO, I2C SCL)-

//---D 2(GPIO, I2C SDA)

//---D 3(GPIO)         
const int ULTRASONIC_TRIGGER = 0; //For ultrasonic trigger

//---D 4(GPIO)         
const int ULTRASONIC_ECHO = 2; //For ultrasonic echo

//---D 5 (GPIO, SPI SCK) 
const int PUMP = 14; //For pump relay

//---D 6 (GPIO, SPI MISO)

//---D 7 (GPIO, SPI MOSI)

//---D 8 (GPIO, SPI SS)

//---A0

//---RX

//---TX

//---Errors---
int Error = 0; //Flag in case of any error => Has to be implemented

//---Constants---
const char* MQTT_CLIENT_ID = "ESP8266Client"; //ID for this MQTT client identification
const char* MQTT_COMMAND   = "MyCommand";     //ID for this MQTT command identification

//---Variables---


//---Functions---
WiFiClient WiFiEspClient;

WiFiServer Server(80);

PubSubClient MQTTClient(WiFiEspClient);


//------------------------------------------------------------------------------


//---Functions------------------------------------------------------------------

  //---SetupWifi---------------------------------------------------------------
  /*   
  Description: Setup the connection to the local Wifi Network.

  Arguments: -
 
  Returns: -
 
  Errors:
  
  Author: Heiko Haegele                                        Date: 05.01.2019
  
  Changed:                                                     Date: dd.mm.yyyy
  
  */
  void SetupWifi()  
  {
 
     delay(10);
 
     Serial.println();  
     Serial.print("Connecting to: ");  
     Serial.println(SSID);    
     WiFi.begin(SSID, PSK);  
 
     while (WiFi.status() != WL_CONNECTED) 
     {
        delay(500);  
        Serial.print(".");
        //---Todo: Make here an emergency exit after a while... 
     }  

     // Start the server 
     Server.begin();  

     Serial.println("WiFi Server started");    
     Serial.println("");  

     Serial.println("IP address: ");  
     Serial.println(WiFi.localIP());

  }   
  //---End SetupWifi-----------------------------------------------------------
 
  //---MQTTConnect------------------------------------------------------------- 
  /*  
  Description: Connects MQTT client in case of lost connection.
 
  Arguments: -
  
  Returns: int                                    0 if failed, else > 1, 2 or 3
  
  Errors:
  
  Author: Heiko Haegele                                        Date: 18.01.2019
  
  Changed:                                                     Date: dd.mm.yyyy
  
  */
  int MQTTConnect()   
  { 
    
    //---Variables---
    int retry = 3; //Number or attempts to connect to MQTT client


    //---Check whether MQTT client is connected, in case if not, connect it---
    if (!MQTTClient.connected())  
    {
      //---Try to connect to MQTT client---
      while (!MQTTClient.connected() && retry != 0)  
      {
      
        Serial.print("Reconnecting MQTT.");
      
        //---Try to connect to MQTT client---
        if (!MQTTClient.connect(MQTT_CLIENT_ID)) //if client has user/pw, then use: connect(MQTT_CLIENT_ID,MQTT_USERNAME,MQTT_PASSWORD))
        {
          retry--;  
          Serial.print("failed, rc=");  
          Serial.print(MQTTClient.state());  
          Serial.println("Retrying in 1 second");  
          
          delay(1000);
        }  
      }
    }

    //---return retry status as information whether its connected or not (its 0 if failed)---
    return retry;       
  }   
  //---End MQTTConnect----------------------------------------------------------

  //---SetupPins----------------------------------------------------------------  
  /*  
  Description: Setup of the IO Pins of the board.
		           This function has to be called in the setup at the beginning.
		           Must be modified according to the current PIN definitions
               (see beginning of this sketch) in of the Project.
  
  Arguments: -
  
  Returns: -
  
  Errors:
  
  Author: Heiko Haegele                                        Date: 06.01.2019
  
  Changed:                                                     Date: dd.mm.yyyy

  */  
  void SetupPins()  
  {
  
     Serial.println("Starting PIN setup.");
    
     pinMode(ULTRASONIC_TRIGGER, OUTPUT);
     pinMode(ULTRASONIC_ECHO, INPUT);
     pinMode(PUMP, OUTPUT);  
 
     Serial.println("PIN Setup finished.");    
  
  }
  //---End SetupPins------------------------------------------------------------

  //---SentToHTTP---------------------------------------------------------------  
  /*  
  Description: Prints the arguments to a http client.
  
  Arguments: -
  
  Returns: -
  
  Errors:
  
  Author: Heiko Haegele                                        Date: 17.01.2019
  
  Changed:                                                     Date: dd.mm.yyyy

  */  
  void SentToHTTP(float LiterCurrentContent)  
  {
  
     Serial.println("Starting SentToHTTP");

    //---HTTP commands to display the Content and graphic on webpage---
     
    //---Check whether there is a WiFi client connected, if not leave main loop---  
    WiFiClient WiFiHTTPClient = Server.available();  
    if (!WiFiHTTPClient)
    { 
      return;  
    }     
  
    //---If there is a client, wait, until its ready to receive data---  
    Serial.println("Wait for Wifi client ready to receive data.");  
    while(!WiFiHTTPClient.available())
    {  
      delay(1);
      //---Todo: Make here an emergency exit after a while... 
    }      
  
    WiFiHTTPClient.println("HTTP/1.1 200 OK");  
    WiFiHTTPClient.println();  
    WiFiHTTPClient.println("<!DOCTYPE html>");  
    WiFiHTTPClient.println("<html xmlns='http://www.w3.org/1999/xhtml'>");  
    WiFiHTTPClient.println("<head>\n<meta charset='UTF-8'>");  
    WiFiHTTPClient.println("<title>Tank level sensor</title>");  
    WiFiHTTPClient.println("</head>\n<body>");  
    WiFiHTTPClient.println("<H2>Tank level:</H2>");  
    WiFiHTTPClient.println("<h3>");  
  
    WiFiHTTPClient.print(LiterCurrentContent);  
    WiFiHTTPClient.println(" Liter");
    Serial.print("LiterCurrentContent[l]: ");  
    Serial.println(LiterCurrentContent);  
       
    WiFiHTTPClient.println("</h3>"); 
    WiFiHTTPClient.print("</body>\n</html>");  
 
    Serial.println("SentToHTTP finished.");    
  
  }
  //---End SentToHTTP-----------------------------------------------------------

//------------------------------------------------------------------------------


//---Main-----------------------------------------------------------------------


  //---Setup--------------------------------------------------------------------  
  /*  
  Description: Initial Setup after start of board.
  
  Arguments: -  

  Returns: -
  
  Errors:
  
  Author: Heiko Haegele                                        Date: 18.01.2019
  
  Changed:                                                     Date: dd.mm.yyyy

  */  
  void setup()  
  {
  
     Serial.begin(115200);
  
     Serial.println("Starting setup.");

     SetupPins();  
    
     SetupWifi();
  
     MQTTClient.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
  
     digitalWrite(PUMP, HIGH); //Activate Pump relay, so that there is power for the Pump
  
     Serial.println("Setup finished.");    
  
  } 
  //---End Setup----------------------------------------------------------------

  
  //---loop---------------------------------------------------------------------  
  /*  
  Description: This is the hopefully infinite loop, which makes the board run.
  
  Arguments: -
  
  Returns: -
  
  Errors:
  
  Author: Heiko Haegele                                        Date: 18.01.2019
  
  Changed:                                                     Date: dd.mm.yyyy
  
  */
  void loop()  
  {

    //---Variables---
    long  EchoDuration = 0;          //[microsec] Echo duration for Ultrasonic signal calculation    
    long  LevelCurrentContent = 0;   //[cm] Measured distance from ultrasonic sensor to the reflecting surface     
    long  LiterCurrentContent = 0;   //[liter] Content
    float PerCentCurrentContent = 0; //[%] Content
    char  MQTTMessage[50];
    
    
    //---Measure distance---  
    digitalWrite(ULTRASONIC_TRIGGER, LOW);  
    delay(5);  
    digitalWrite(ULTRASONIC_TRIGGER, HIGH);  
    delay(10);
    digitalWrite(ULTRASONIC_TRIGGER, LOW);
        
    EchoDuration = pulseIn(ULTRASONIC_ECHO, HIGH);
    Serial.print("EchoDuration[us]: ");  
    Serial.println(EchoDuration);   

    //---Check if ultrasonic got the measurements correct, timeout value when more than 999999 microsec---
    if (EchoDuration >= 999999 || EchoDuration <= 0)   
    {  
      //---Measurements failed---  
      Serial.println("Ultrasonic measurements error");  

      Error = 1; //Make use somewhere of this flags....  
    }  
    else  
    {  
      //---Measurements correct, calculate content---  
      Serial.println("Ultrasonic measurements ok");           
       
      LevelCurrentContent = LEVEL_MAX_CONTENT - (EchoDuration * 0.034 / 2) - DISTANCE_SENSOR;   //0.034 is sonic velocity, /2 because duration is the time to way to the bottom and back to sensor      
      Serial.print("LevelCurrentContent[cm]: ");  
      Serial.println(LevelCurrentContent);

      LiterCurrentContent = (3.1416 * (DIAMETER/2) * (DIAMETER/2) * LevelCurrentContent) / 1000; //my tank is a cylinder /1000 to make liter from ccm
      Serial.print("LiterCurrentContent[l]: ");  
      Serial.println(LiterCurrentContent);
      
      PerCentCurrentContent = 100 / LEVEL_MAX_CONTENT * LevelCurrentContent;
      Serial.print("PerCentCurrentContent[%]: ");  
      Serial.println(PerCentCurrentContent);                  
    }  
   
    //---If content too low, disable pump---  
    if(LevelCurrentContent < LEVEL_PUMP_ON) 
    {  
       Serial.println("Switch off pump, contentlevel too low");  
       digitalWrite(PUMP, LOW); //If there is too low Content, deactivate Pump relay, so there is no power for the Pump  
    }

    //---Connect to MQTT client and send the data---
    if (MQTTConnect())  
    {         

      //---Subscribe to MQTT command---
      MQTTClient.subscribe(MQTT_COMMAND);
      
      MQTTClient.loop();     
      
      snprintf (MQTTMessage, 50, "LiterCurrentContent: ", LiterCurrentContent);
      
      Serial.print("MQTT message: ");  
      Serial.println(MQTTMessage);
      
      MQTTClient.publish(MQTT_TOPIC_OUT, MQTTMessage);
     
      Serial.println("Waiting to enable reprogramming.");  
      //Serial.println("Deepsleep + related Delay temporary disabled");
      delay(30000); //time to make reprogramming possible, before falling into deepsleep 
      Serial.println("Nothing to do. So... deepsleep.......zzzzzzz"); 
      ESP.deepSleep(DEEPSLEEP_MINUTES * 60 * 1000000);  
      delay(100); //for save deepsleep

    }
    else
    {
      
      SentToHTTP(LiterCurrentContent); //Send the calculated content to HTTP for displaying in browser
  
      delay(500);      
    }
  
  }
  //---End loop-----------------------------------------------------------------

//------------------------------------------------------------------------------

//---End File Name--------------------------------------------------------------
