// Sensor Mega! V.001
// Receives Data from Xbee Coordinator and Serial1 Arduino UNO. 
// Processes numerous other Sensors and uploads to Xively and local SQL via Rasberry Pi
// http://www.dangertech.org

#include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
#include <Xively.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>
#include <TSL2561.h>
#include <XBee.h>
#include <Time.h>
#include <DS1307RTC.h>

//BMP085 Pressure
Adafruit_BMP085 bmp;

//DHT22
#define DHTPIN 5     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

//lux
TSL2561 tsl(TSL2561_ADDR_LOW);

//xbee
XBee xbee = XBee();
ZBRxResponse rx = ZBRxResponse();
ZBRxIoSampleResponse ioSample = ZBRxIoSampleResponse();
XBeeAddress64 test = XBeeAddress64();

//ethernet
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Your Xively key to let you upload data
char xivelyKey[] = "8q9hxZHPQggWKsXqPEBZymEHa5Uyfiwv1TrcPXBenKPCsCr7";

// Define the strings for our datastream IDs
char sensorId0[] = "Comms_Motion";
char sensorId1[] = "Comms_Temp";
char sensorId2[] = "Comms_Barometer";
char sensorId3[] = "Comms_Humidity";
char sensorId4[] = "Comms_Lux";
char sensorId5[] = "Outdoor_Temp";
char sensorId6[] = "Outdoor_Voltage";
char sensorId7[] = "Xbee1_Battery";
char sensorId8[] = "Xbee1_Solar";
char sensorId9[] = "Total_Power_Use";
char sensorId10[] = "Hydroheat";
char sensorId11[] = "Lights_Powah";
char sensorId12[] = "Water_Usage";
char sensorId13[] = "Water_Usage_Hourly";
char sensorId14[] = "Water_Usage_Daily";
char sensorId15[] = "Bedroom1_Temp";
char sensorId16[] = "Laundry_Temp";
//char bufferId[] = "info_message";
//String stringId("random_string");
const int bufferSize = 140;
char bufferValue[bufferSize]; // enough space to store the string we're going to send

XivelyDatastream datastreams[] = {
  XivelyDatastream(sensorId0, strlen(sensorId0), DATASTREAM_INT),
  XivelyDatastream(sensorId1, strlen(sensorId1), DATASTREAM_FLOAT),
  XivelyDatastream(sensorId2, strlen(sensorId2), DATASTREAM_FLOAT),
  XivelyDatastream(sensorId3, strlen(sensorId3), DATASTREAM_FLOAT),
  XivelyDatastream(sensorId4, strlen(sensorId4), DATASTREAM_INT),
  XivelyDatastream(sensorId5, strlen(sensorId5), DATASTREAM_FLOAT),
  XivelyDatastream(sensorId6, strlen(sensorId6), DATASTREAM_FLOAT),
  XivelyDatastream(sensorId7, strlen(sensorId7), DATASTREAM_FLOAT),
  XivelyDatastream(sensorId8, strlen(sensorId8), DATASTREAM_FLOAT),
  XivelyDatastream(sensorId9, strlen(sensorId9), DATASTREAM_INT),
  XivelyDatastream(sensorId10, strlen(sensorId10), DATASTREAM_INT),
  XivelyDatastream(sensorId11, strlen(sensorId11), DATASTREAM_INT),
  XivelyDatastream(sensorId12, strlen(sensorId12), DATASTREAM_FLOAT),
  XivelyDatastream(sensorId13, strlen(sensorId13), DATASTREAM_FLOAT),
  XivelyDatastream(sensorId14, strlen(sensorId14), DATASTREAM_FLOAT),
  XivelyDatastream(sensorId15, strlen(sensorId15), DATASTREAM_FLOAT),
  XivelyDatastream(sensorId16, strlen(sensorId16), DATASTREAM_FLOAT),
  // XivelyDatastream(bufferId, strlen(bufferId), DATASTREAM_BUFFER, bufferValue, bufferSize),
  // XivelyDatastream(stringId, DATASTREAM_STRING)
};
// Finally, wrap the datastreams into a feed
XivelyFeed feed(1177751918, datastreams, 16 /* number of datastreams */);

EthernetClient client;
XivelyClient xivelyclient(client);

int sensorPin = 0; //light
const int motionPin = 2; // choose the input pin (for PIR sensor)
const int ledPin = 3; //LED
const int foyeurLedPin = 7; //Foyeur Motion ON LED
int timer = 0;
const int hydroLED = 6; //LED that comes on with hotwater/heatpump

float strWater = 0;
int waterTimer = 0;
int waterTimer2 = 0;
float waterHourly;
float waterDaily;

char server[] = "http://emoncms.org/";     //emoncms URL
String apiKey = "ebd4f194e60f6e8694f56aa48b094ddb";

//powerserial1
const int fNumber = 3; //number of fields to recieve in data stream
int fieldIndex =0; //current field being recieved
int values[fNumber]; //array holding values

String xbeeReadString = "";
String xbeeReadString2 = "";

String realPower1 = "";
String realPower2 = "";
String realPower3 = "";
String realPower4 = "";
float realPower5 = 0;
String Irms1 = "";
String Irms2 = "";
String Irms3 = "";
String Irms4 = "";
String Vrms = "";

String foyeurLux = "";
String hotWaterHot = "";
String hotWaterCold = "";
String foyeurHumidity = "";
String foyeurTemp = "";
String FoyeurMotion = "";

int packetSize = xbeeReadString.length(); 

//***************************************************
void setup() {
  Serial.begin(19200);  //Debug
  Serial1.begin(9600); //Currentcost chat
  Serial2.begin(9600); //Xbee chat
  Serial3.begin(9600); //Output to pi

  xbee.setSerial(Serial2); //sets serial2 to be used for xbee library

  Serial.println("Starting SensorNet...");
  Serial.println();

  //Ethernet OK?
  while (Ethernet.begin(mac) != 1)
  {
    Serial.println("Error getting IP address via DHCP, trying again...");
    delay(15000);
  }
  Serial.print("IP address: ");
  Ethernet.localIP().printTo(Serial);
  Serial.println();

  Serial.print("Gateway IP address is ");
  Ethernet.gatewayIP().printTo(Serial);
  Serial.println();

  Serial.print("DNS IP address is ");
  Ethernet.dnsServerIP().printTo(Serial);
  Serial.println();

  Serial.println("Ethernet OK....");
  //Barometer OK?
  //time for barometer to start up
  Serial.println("Barometer Warmup Phase..."); 
  delay(1000); 
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {
    }
  }
  //Lux  
  //tsl.setGain(TSL2561_GAIN_0X);         // set no gain (for bright situtations)
  tsl.setGain(TSL2561_GAIN_16X);      // set 16x gain for dim situations
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  tsl.setTiming(TSL2561_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_402MS);  // longest integration time (dim light)

  Serial.println("Barometer OK.....");

  pinMode(motionPin, INPUT);     // declare sensor as input
  pinMode(ledPin, OUTPUT);
  pinMode(hydroLED, OUTPUT);
  pinMode(foyeurLedPin, OUTPUT);
  digitalWrite(motionPin, LOW);
  digitalWrite(hydroLED, LOW);
  digitalWrite(foyeurLedPin, LOW);
  
  Serial.println();
  
  
  //RTX Time Sync
    setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if(timeStatus()!= timeSet) 
     Serial.println("Unable to sync with the RTC");
  else
     Serial.println("RTC has set the system time");   
      digitalClockDisplay();   
}



void loop() {
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  
  int commsMotion = digitalRead(motionPin);
  int pirOut = 0;
  if (commsMotion == HIGH) {
    digitalWrite(ledPin, HIGH);
    commsMotion = 1;
  }
  datastreams[0].setInt(commsMotion);

 // if (FoyeurMotion == 1) {
  //  digitalWrite(foyeurLedPin, HIGH);
 //     }
 
  timer ++;
  // Serial.println(timer);
  delay(10);
  //xbee
  //attempt to read a packet    
  xbee.readPacket();

  if (xbee.getResponse().isAvailable()) {
    if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
      // got a zb rx packet, the kind this code is looking for
      // now that you know it's a receive packet
      // fill in the values
      xbee.getResponse().getZBRxResponse(rx);

      // this is how you get the 64 bit address out of
      // the incoming packet so you know which device
      // it came from
      Serial.print("Got an rx packet from: ");
      XBeeAddress64 senderLongAddress = rx.getRemoteAddress64();
      print32Bits(senderLongAddress.getMsb());
      Serial.print(" ");
      print32Bits(senderLongAddress.getLsb());

      // this is how to get the sender's
      // 16 bit address and show it
      uint16_t senderShortAddress = rx.getRemoteAddress16();
      Serial.print(" (");
      print16Bits(senderShortAddress);
      Serial.println(")");

      Serial.print(" ");
      Serial.print(senderLongAddress.getLsb());
      uint32_t xbee = (senderLongAddress.getLsb());  

      // The option byte is a bit field
      if (rx.getOption() & ZB_PACKET_ACKNOWLEDGED)
        // the sender got an ACK
        Serial.println("packet acknowledged");
      if (rx.getOption() & ZB_BROADCAST_PACKET)
        // This was a broadcast packet
        Serial.println("broadcast Packet");

      Serial.print("checksum is ");
      Serial.print(rx.getChecksum(), HEX);
      Serial.print(" ");

      // this is the packet length
      Serial.print("packet length is ");
      Serial.print(rx.getPacketLength(), DEC);

      // this is the payload length, probably
      // what you actually want to use
      Serial.print(", data payload length is ");
      Serial.println(rx.getDataLength(),DEC);

      // this is the actual data you sent
      Serial.println("Received Data: ");
      for (int i = 0; i < rx.getDataLength(); i++) {
        print8Bits(rx.getData()[i]);
        Serial.print(" ");
      }
      /*   // and an ascii representation for those of us
       // that send text through the XBee
       Serial.println();
       for (int i= 0; i < rx.getDataLength(); i++){
       //     Serial.write(' ');
       if (iscntrl(rx.getData()[i]));
       //       Serial.write(' ');
       else
       Serial.print(rx.getData()[i]);
       //     Serial.write(' ');
       }
       */
      Serial.println();
      // So, for example, you could do something like this:
      handleXbeeRxMessage(rx.getData(), rx.getDataLength());
      Serial.print("xbeereadstring:");
      Serial.println(xbeeReadString);
      packetSize = xbeeReadString.length(); 

      if(xbee == 1084373003) { //Watermeter
        Serial.println("=========Water Meter=========");
        waterTimer ++;
        waterTimer2 ++;
        String water = xbeeReadString.substring(19, 25);
        char floatbuf[8]; // make this at least big enough for the whole string
        water.toCharArray(floatbuf, sizeof(floatbuf));
        strWater = atof(floatbuf);
        waterHourly = waterHourly + strWater;
        waterDaily = waterDaily + strWater;
        datastreams[12].setFloat(strWater);
        datastreams[13].setFloat(waterHourly);
        datastreams[14].setFloat(waterDaily);
        Serial.print("water use:");
        Serial.print(strWater);
        Serial.println("L/min");
        Serial.print("water use:");
        Serial.print(waterHourly);
        Serial.println("L/hour");
        Serial.print("water use:");
        Serial.print(waterDaily);
        Serial.println("L/day");
        Serial.println("===========================");
        Serial2.flush();
        xbeeReadString = "";
        xbeeReadString2 = "";
        if(waterTimer2 == 60) { //Need to introduce a RTC to sync times with RL hours etc
          waterHourly = 0;
          waterTimer2 = 0;
        }
        if(waterTimer == 1440) { //resets Daily count after 1440 minutes (24 Hours)
          waterDaily = 0;
          waterTimer = 0;
        }
      }

      if(xbee == 1081730785 && packetSize > 40) { //Foyeur
        Serial.println("=========Foyeur=========");
        String xbeeReadString2 = xbeeReadString.substring(17, 48);
        Serial.print("String1=");
        Serial.println(xbeeReadString);
        Serial.print("String2=");
        Serial.println(xbeeReadString2); //String2=57.25,18.00,18.00,58.20,18.20,0õ

        foyeurLux = xbeeReadString2.substring(0, 5);
        hotWaterHot = xbeeReadString2.substring(6, 11);
        hotWaterCold = xbeeReadString2.substring(12, 17);
        foyeurHumidity = xbeeReadString2.substring(18, 23);
        foyeurTemp = xbeeReadString2.substring(24, 29);
        FoyeurMotion = xbeeReadString2.substring(30, 31);

        foyeurLux.trim();
        hotWaterHot.trim();
        hotWaterCold.trim();
        foyeurHumidity.trim();
        foyeurTemp.trim();

        Serial.print("Light Lux: ");         
        Serial.println(foyeurLux);
        Serial.print("Hot Water Hot: ");         
        Serial.println(hotWaterHot);
        Serial.print("Hot Water Cold: ");         
        Serial.println(hotWaterCold);
        Serial.print("Foyeur Humidity: ");         
        Serial.println(foyeurHumidity);
        Serial.print("Foyeur Temp: ");         
        Serial.println(foyeurTemp);
        Serial.print("Foyeur Motion: ");         
        Serial.println(FoyeurMotion);

        xbeeReadString = "";
        xbeeReadString2 = "";

      }

      if(xbee == 1081730785 && packetSize < 35) {
        Serial.println("=========Foyeur=========");
        String xbeeReadString2 = xbeeReadString.substring(17, 32);
        Serial.print("String1=");
        Serial.println(xbeeReadString);
        Serial.print("String2=");
        Serial.println(xbeeReadString2); //String2=57.25,18.00,18.00,58.20,18.20,0õ
        String FoyeurMotion = xbeeReadString2.substring(0, 1);
        Serial.print("Foyeur Motion: ");         
        Serial.println(FoyeurMotion);
        xbeeReadString = "";
        xbeeReadString2 = "";
      }


      if(xbee == 1081730797 && packetSize > 20) { //powermeter
        Serial.println("=========Power Meter=========");
        Serial.print("Packet Size: ");         
        Serial.println(packetSize,DEC);
        String xbeeReadString2 = xbeeReadString.substring(17, 83);
        Serial.print("String1=");
        Serial.println(xbeeReadString);
        Serial.print("String2=");
        Serial.println(xbeeReadString2);

        realPower1 = xbeeReadString2.substring(0, 8);
        realPower2 = xbeeReadString2.substring(9, 17);
        realPower3 = xbeeReadString2.substring(18, 26);
        realPower4 = xbeeReadString2.substring(27, 35);
        Irms1 = xbeeReadString2.substring(36, 41);
        Irms2 = xbeeReadString2.substring(42, 47);
        Irms3 = xbeeReadString2.substring(48, 53);
        Irms4 = xbeeReadString2.substring(54, 59);
        Vrms = xbeeReadString2.substring(60, 66);

        realPower1.trim();
        realPower2.trim();
        realPower3.trim();
        realPower4.trim();
        Irms1.trim();
        Irms2.trim();
        Irms3.trim();
        Irms4.trim();

        //Need to write a function for this!
        char floatbuf[10]; // make this at least big enough for the whole string
        realPower1.toCharArray(floatbuf, sizeof(floatbuf));
        float fltPower1 = atof(floatbuf);
        realPower2.toCharArray(floatbuf, sizeof(floatbuf));
        float fltPower2 = atof(floatbuf);
        realPower3.toCharArray(floatbuf, sizeof(floatbuf));
        float fltPower3 = atof(floatbuf);
        realPower4.toCharArray(floatbuf, sizeof(floatbuf));
        float fltPower4 = atof(floatbuf);
        float fltPower5 = fltPower4 - fltPower3; // Calculating Lights and Powerpoints Usage.





        //EmonCMS
        Serial.println("Connecting.....");
        if(client.connect("emoncms.org",80)){
          client.print("GET /input/post.json?json={TotalPower:");  // make sure there is a [space] between GET and /input
          //client.print("TotalPower:");
          client.print(fltPower4);
          client.print(",Solar:");
          client.print(fltPower1);  
          client.print(",PowerP:");
          client.print(fltPower2);
          client.print(",HotwaterHeater:");
          client.print(fltPower3);
          client.print(",PowerandLights:");
          client.print(fltPower5);
          client.print(",TotalCurrent:");
          client.print(Irms4);
          client.print(",SolarCurrent:");
          client.print(Irms1);  
          client.print(",PowerPCurrent:");
          client.print(Irms2);
          client.print(",HydroCurrent:");
          client.print(Irms3);
          client.print(",LineVoltage:");
          client.print(Vrms);
          client.print("}&apikey=");
          client.print(apiKey);         //assuming APIKEY is a char or string
          client.println(" HTTP/1.1");   //make sure there is a [space] BEFORE the HTTP
          client.println("Host: emoncms.org");
          client.println("User-Agent: Arduino-ethernet");
          client.println("Connection: close");     //    Although not technically necessary, I found this helpful
          client.println();
          Serial.println("****EmonCMS Logged****");
          client.stop();
        }
        else {
          Serial.println("*************Upload to EmonCMS Failed *************");
          client.stop();
        }

        Serial.print("CT1 Solar:");
        Serial.print(realPower1);

        Serial.print("  CT2 Spare: ");
        Serial.print(realPower2);
        Serial.print("  CT3 Hydro: ");
        Serial.print(realPower3);
        Serial.print("  CT4 Total: ");
        Serial.print(realPower4);
        Serial.print("  PP/Lights: ");
        Serial.println(realPower5);

        Serial.print("CT1 Current:");
        Serial.print(Irms1);
        Serial.print("  CT2 Current:");
        Serial.print(Irms2);
        Serial.print("  CT3 Current:");
        Serial.print(Irms3);
        Serial.print("  CT4 Current:");
        Serial.println(Irms4);

        Serial.print("Line Voltage:");
        Serial.println(Vrms);
        Serial.println("===========================");

        Serial2.flush();
        xbeeReadString = "";
        xbeeReadString2 = "";

      }


    }
    // XBEE IO Samples
    else if (xbee.getResponse().getApiId() == ZB_IO_SAMPLE_RESPONSE) {
      xbee.getResponse().getZBRxIoSampleResponse(ioSample);
      XBeeAddress64 senderLongAddress = ioSample.getRemoteAddress64();
      Serial.println(senderLongAddress.getLsb());
      uint32_t xbee = (senderLongAddress.getLsb());

      if (ioSample.containsAnalog()) {
        // Serial.println("Sample contains analog data");
        Serial.println("Received I/O Sample from: ");
        // this is how you get the 64 bit address out of
        // the incoming packet so you know which device
        // it came from
        uint8_t bitmask = ioSample.getAnalogMask();
        for (uint8_t x = 0; x < 8; x++){
          if ((bitmask & (1 << x)) != 0){
            Serial.print("position ");
            Serial.print(x, DEC);
            Serial.print(" value: ");
            Serial.print(ioSample.getAnalog(x));
            Serial.println();
          }
        }
      }
      if (xbee == 1083188734) {
        Serial.println("==========Outside==========");
        int reading = (ioSample.getAnalog(0));
        float voltage = reading * 1.2;
        voltage /= 1024.0; 
        float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
        //to degrees ((volatge - 500mV) times 100)
        Serial.print(temperatureC); 
        Serial.println(" degrees C");
        datastreams[5].setFloat(temperatureC);

        int vReading = (ioSample.getAnalog(1));
        float xbee1battery = vReading * 4.2 / 1024;      
        // voltage /= 1024.0; 
        Serial.print(xbee1battery); 
        Serial.println(" Xbee Battery");
        datastreams[7].setFloat(xbee1battery);

        int vReading2 = (ioSample.getAnalog(2));
        float xbee1solar = vReading2 * 6.0 / 1024;      
        // voltage /= 1024.0; 
        Serial.print(xbee1solar); 
        Serial.println(" Xbee Solar");
        datastreams[8].setFloat(xbee1solar);

        int vReading3 = (ioSample.getAnalog(7));
        float xbee1v = vReading3 * 1.2 / 1024;      
        // voltage /= 1024.0; 
        Serial.print(xbee1v); 
        Serial.println(" Xbee Voltage");
        datastreams[6].setFloat(xbee1v);
        Serial.println("===========================");
        Serial2.flush();
      }
      if (xbee == 1081730917) {
        Serial.println("==========Bedroom==========");
        int reading = (ioSample.getAnalog(0));
        float voltage = reading * 1.2;
        voltage /= 1024.0; 
        float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
        //to degrees ((volatge - 500mV) times 100)
        Serial.print(temperatureC); 
        Serial.println(" degrees C");
        datastreams[15].setFloat(temperatureC);

        int vReading3 = (ioSample.getAnalog(7));
        float xbee1v = vReading3 * 1.2 / 1024;      
        // voltage /= 1024.0; 
        Serial.print(xbee1v); 
        Serial.println(" Bedroom Xbee Voltage Not logged");
        Serial.println("===========================");
        Serial2.flush();
      }

      if (xbee == 1082562186) {
        Serial.println("==========Tester==========");
        int reading = (ioSample.getAnalog(0));
        int vReading3 = (ioSample.getAnalog(7));
        float xbee1v = vReading3 * 1.2 / 1024;      
        Serial.print(xbee1v); 
        Serial.println(" Test Xbee Voltage Not logged");
        Serial2.flush();
        Serial.println("===========================");
      }

      if (xbee == 1083645575) {
        Serial.println("==========Laundry==========");
        int reading = (ioSample.getAnalog(0));
        float voltage = reading * 1.2;
        voltage /= 1024.0; 
        float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
        //to degrees ((volatge - 500mV) times 100)
        Serial.print(temperatureC); 
        Serial.print(" degrees C ");
        datastreams[16].setFloat(temperatureC);

        int vReading3 = (ioSample.getAnalog(7));
        float xbee1v = vReading3 * 1.2 / 1024;      
        Serial.print(xbee1v); 
        Serial.println(" Xbee Voltage Not logged");
        Serial2.flush();     
        Serial.println("===========================");
      }
    }

    else {
      Serial.print("Expected I/O Sample, but got ");
      Serial.print(xbee.getResponse().getApiId(), HEX);
    }   
  }

  else if (xbee.getResponse().isError()) {
    Serial.print("Error reading packet.  Error code: ");  
    Serial.println(xbee.getResponse().getErrorCode());  
    Serial2.flush();
  }




  //*****************************************************
  //         Power receipt
  //*****************************************************


  if (timer >= 5000) {
    Serial.println("==========CurrentCost==========");
    Serial1.write("S");
    Serial.println();
    Serial.println("Power Request Sent...");
    // if (Serial1.available()) {
    for(fieldIndex = 0; fieldIndex  < 3; fieldIndex ++)
    {
      values[fieldIndex] = Serial1.parseInt(); 
    }
    Serial.print(fieldIndex);
    Serial.println(" fields received: ");
    for(int i=0; i <  fieldIndex; i++)
    {
      //   Serial.println(values[i]);
      if(values[0]) {
        // Serial.print("totalpower: "); Serial.println(values[0]);
        datastreams[9].setInt(values[0]);
      }
      if(values[1]){
        // Serial.print("hydro: ");   Serial.println(values[1]);
        datastreams[10].setInt(values[1]);
        if(values[1] > 5) {
          digitalWrite(hydroLED, HIGH);
        }
        else{
          digitalWrite(hydroLED, LOW);
        }
      }
      if (values[2]){
        // Serial.print("lightsandpowah: "); Serial.println(values[2]);
        datastreams[11].setInt(values[2]);
      }
    }

    Serial.print("totalpower: "); 
    Serial.println(values[0]);
    Serial.print("hydro: ");   
    Serial.println(values[1]);
    Serial.print("lightsandpowah: "); 
    Serial.println(values[2]);
    Serial.println();

    fieldIndex = 0; //reset
    Serial.println("===========================");
    Serial1.flush();
    //  }
  }

  if (second() == 59) {

    float barometerTemp = (bmp.readTemperature());
    Serial.print("Barometer Temperature = ");
    Serial.print(barometerTemp);
    Serial.println(" *C");
    datastreams[1].setFloat(barometerTemp);

    float barometerPressure = (bmp.readPressure());
    Serial.print("Pressure = ");
    Serial.print(barometerPressure);
    Serial.println(" Pa");
    datastreams[2].setFloat(barometerPressure);

    Serial.print("Humidity: "); 
    Serial.print(dht.readHumidity());
    Serial.println(" %\t");
    datastreams[3].setFloat(dht.readHumidity());

    Serial.print("Humid Temperature = ");
    Serial.print(dht.readTemperature());
    Serial.println(" *C Not Logged");


    //Luminosity Full Visible calcs
    Serial.print("Full LUX = ");
    uint16_t x = tsl.getLuminosity(TSL2561_VISIBLE);   
    uint32_t lum = tsl.getFullLuminosity();
    uint16_t ir, full;
    ir = lum >> 16;
    full = lum & 0xFFFF;
    Serial.println(full);
    datastreams[4].setInt(full);

    Serial.print("lightsandpowah: "); 
    Serial.println(values[2]);
    Serial.print("hydro: ");   
    Serial.println(values[1]);
    Serial.print("totalpower: "); 
    Serial.println(values[0]);

    //SQL feed
    Serial.println("SQL:");
    //Serial 3 to Pi
    Serial3.print(datastreams[0].getInt());
    Serial3.print(",");
    Serial3.print(datastreams[1].getFloat());
    Serial3.print(",");
    Serial3.print(datastreams[2].getFloat());
    Serial3.print(",");
    Serial3.print(datastreams[3].getFloat());
    Serial3.print(",");
    Serial3.print(datastreams[4].getInt());
    Serial3.print(",");
    Serial3.print(datastreams[5].getFloat());
    Serial3.print(",");
    Serial3.print(datastreams[6].getFloat());
    Serial3.print(",");
    Serial3.print(datastreams[7].getFloat());
    Serial3.print(",");
    Serial3.print(datastreams[8].getFloat());
    Serial3.print(",");
    Serial3.print(datastreams[9].getInt());
    Serial3.print(",");
    Serial3.print(datastreams[10].getInt());
    Serial3.print(",");
    Serial3.print(datastreams[11].getInt());
    // Serial3.print(",");

    //debug console   
    Serial.print(datastreams[0].getInt());
    Serial.print(",");
    Serial.print(datastreams[1].getFloat());
    Serial.print(",");
    Serial.print(datastreams[2].getFloat());
    Serial.print(",");
    Serial.print(datastreams[3].getFloat());
    Serial.print(",");
    Serial.print(datastreams[4].getInt());
    Serial.print(",");
    Serial.print(datastreams[5].getFloat());
    Serial.print(",");
    Serial.print(datastreams[6].getFloat());
    Serial.print(",");
    Serial.print(datastreams[7].getFloat());
    Serial.print(",");
    Serial.print(datastreams[8].getFloat());
    Serial.print(",");
    Serial.print(datastreams[9].getInt());
    Serial.print(",");
    Serial.print(datastreams[10].getInt());
    Serial.print(",");
    Serial.print(datastreams[11].getInt());
    Serial.print(",");
    Serial.print(datastreams[12].getFloat());
    Serial.println();
    Serial.println("SQL Injected!");
    Serial.println();

    Serial.println("Uploading it to Xively");
    int ret = xivelyclient.put(feed, xivelyKey);
    Serial.print("xivelyclient.put returned ");
    Serial.println(ret);


    //EmonCMS
    if(client.connect("80.243.190.58",80)){
      Serial.println("Connecting.....");
      Serial.println("==========EMONCMS==========");
      client.print("GET /input/post.json?json={CommsMotion:");  // make sure there is a [space] between GET and /input
      //   client.print("CommsMotion:");
      client.print(datastreams[0].getInt());
      client.print(",CommsTemp:");
      client.print(datastreams[1].getFloat());
      client.print(",CommsBarometer:");
      client.print(datastreams[2].getFloat());  
      client.print(",CommsHumidity:");
      client.print(datastreams[3].getFloat());
      client.print(",CommsLux:");
      client.print(datastreams[4].getInt());
      client.print(",TempOutside:");
      client.print(datastreams[5].getFloat());
      client.print(",Xbee1InternalV:");
      client.print(datastreams[6].getFloat());  
      client.print(",Xbee1BatteryV:");
      client.print(datastreams[7].getFloat());
      client.print(",Xbee1SolarV:");
      client.print(datastreams[8].getFloat());
      client.print(",HousePowerUse:");
      client.print(datastreams[9].getInt());  
      client.print(",HeaterHotwaterUse:");
      client.print(datastreams[10].getInt());
      client.print(",LightsPowerUse:");
      client.print(datastreams[11].getInt());
      client.print(",WaterusageMinute:");
      client.print(datastreams[12].getFloat());
      client.print(",WaterusageHourly:");
      client.print(datastreams[13].getFloat());  
      client.print(",WaterusageDaily:");
      client.print(datastreams[14].getFloat());
      client.print(",Bedroom1Temp:");
      client.print(datastreams[15].getFloat());
      client.print(",LaundryTemp:");
      client.print(datastreams[16].getFloat());
      client.print(",FoyeurLux:");
       client.print(foyeurLux);
       client.print(",FoyeurHumidity:");
       client.print(foyeurHumidity);  
       client.print(",FoyeurTemp:");
       client.print(foyeurTemp);
       client.print(",FoyeurMotion:");
       client.print(FoyeurMotion);
       client.print("}&apikey=");
      client.print(apiKey);         //assuming APIKEY is a char or string
      client.println(" HTTP/1.1");   //make sure there is a [space] BEFORE the HTTP
      client.println("Host: emoncms.org");
      client.println("User-Agent: Arduino-ethernet");
      client.println("Connection: close");     //    Although not technically necessary, I found this helpful
      client.println();
      client.stop();

      Serial.print("GET /input/post.json?json={");  // make sure there is a [space] between GET and /input
      Serial.print("CommsMotion:");
      Serial.print(datastreams[0].getInt());
      Serial.print(",CommsTemp:");
      Serial.print(datastreams[1].getFloat());
      Serial.print(",CommsBarometer:");
      Serial.print(datastreams[2].getFloat());  
      Serial.print(",CommsHumidity:");
      Serial.print(datastreams[3].getFloat());
      Serial.print(",CommsLux:");
      Serial.print(datastreams[4].getInt());
      Serial.print(",TempOutside:");
      Serial.print(datastreams[5].getFloat());
      Serial.print(",Xbee1InternalV:");
      Serial.print(datastreams[6].getFloat());  
      Serial.print(",Xbee1BatteryV:");
      Serial.print(datastreams[7].getFloat());
      Serial.print(",Xbee1SolarV:");
      Serial.print(datastreams[8].getFloat());
      Serial.print(",HousePowerUse:");
      Serial.print(datastreams[9].getInt());  
      Serial.print(",HeaterHotwaterUse:");
      Serial.print(datastreams[10].getInt());
      Serial.print(",LightsPowerUse:");
      Serial.print(datastreams[11].getInt());
      Serial.print(",WaterusageMinute:");
      Serial.print(datastreams[12].getFloat());
      Serial.print(",WaterusageHourly:");
      Serial.print(datastreams[13].getFloat());  
      Serial.print(",WaterusageDaily:");
      Serial.print(datastreams[14].getFloat());
      Serial.print(",Bedroom1Temp:");
      Serial.print(datastreams[15].getFloat());
      Serial.print(",LaundryTemp:");
      Serial.print(datastreams[16].getFloat());

      Serial.print(",TotalPower:");
      Serial.print(realPower4);
      Serial.print(",Solar:");
      Serial.print(realPower1);  
      Serial.print(",PowerP:");
      Serial.print(realPower2);
      Serial.print(",HotwaterHeater:");
      Serial.print(realPower3);
      Serial.print(",PowerandLights:");
      Serial.print(realPower5);
      Serial.print(",TotalCurrent:");
      Serial.print(Irms4);
      Serial.print(",SolarCurrent:");
      Serial.print(Irms1);  
      Serial.print(",PowerPCurrent:");
      Serial.print(Irms2);
      Serial.print(",HydroCurrent:");
      Serial.print(Irms3);
      Serial.print(",LineVoltage:");
      Serial.print(Vrms);
      Serial.print("}&apikey=");
      Serial.println(apiKey);         //assuming APIKEY is a char or string

      Serial.println(" HTTP/1.1");   //make sure there is a [space] BEFORE the HTTP
      Serial.println(F("Host: emoncms.org"));
      Serial.println(F("User-Agent: Arduino-ethernet"));
      Serial.println(F("Connection: close"));     //    Although not technically necessary, I found this helpful
      Serial.println();

      Serial.println("Upload to EmonCMS Completed");
      Serial.println("===========================");
    }
    else {
      Serial.println("Upload to EmonCMS Failed *************");
      client.stop();
    }

    //reset comms motion switch here to update interval for motion detected not just to update if commsmotion and activity update coincides like old way
    digitalWrite(ledPin, LOW);
    commsMotion = 0;
    timer = 0;
    Serial.println();
    Serial.println("end");
  }
}

void handleXbeeRxMessage(uint8_t *data, uint8_t length){
  // this is just a stub to show how to get the data,
  // and is where you put your code to do something with
  // it.
  for (int i = 0; i < length; i++){
    //  char try[80];
    char xbuff = data[i];
    xbeeReadString += xbuff;
    //   Serial.print(final,DEC);
    // Serial.print(data[i]);
  }
  Serial.println();
}

void showFrameData(){
  Serial.println("Incoming frame data:");
  for (int i = 0; i < xbee.getResponse().getFrameDataLength(); i++) {
    print8Bits(xbee.getResponse().getFrameData()[i]);
    Serial.print(' ');
  }
  Serial.println();
  for (int i= 0; i < xbee.getResponse().getFrameDataLength(); i++){
    Serial.write(' ');
    if (iscntrl(xbee.getResponse().getFrameData()[i]))
      Serial.write(' ');
    else
      Serial.write(xbee.getResponse().getFrameData()[i]);
    Serial.write(' ');
  }
  Serial.println(); 
}

// these routines are just to print the data with
// leading zeros and allow formatting such that it
// will be easy to read.
void print32Bits(uint32_t dw){
  print16Bits(dw >> 16);
  print16Bits(dw & 0xFFFF);
}

void print16Bits(uint16_t w){
  print8Bits(w >> 8);
  print8Bits(w & 0x00FF);
}

void print8Bits(byte c){
  uint8_t nibble = (c >> 4);
  if (nibble <= 9)
    Serial.write(nibble + 0x30);
  else
    Serial.write(nibble + 0x37);

  nibble = (uint8_t) (c & 0x0F);
  if (nibble <= 9)
    Serial.write(nibble + 0x30);
  else
    Serial.write(nibble + 0x37);
}
void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}


























