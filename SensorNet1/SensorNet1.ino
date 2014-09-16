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
  // XivelyDatastream(bufferId, strlen(bufferId), DATASTREAM_BUFFER, bufferValue, bufferSize),
  // XivelyDatastream(stringId, DATASTREAM_STRING)
};
// Finally, wrap the datastreams into a feed
XivelyFeed feed(1177751918, datastreams, 15 /* number of datastreams */);

EthernetClient client;
XivelyClient xivelyclient(client);

int sensorPin = 0; //light
int motionPin = 2; // choose the input pin (for PIR sensor)
int ledPin = 3; //LED
int timer = 0;
int hydroLED = 6; //LED that comes on with hotwater/heatpump

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

//***************************************************
void setup() {
  Serial.begin(9600);  //Debug
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
  digitalWrite(motionPin, LOW);
  digitalWrite(hydroLED, LOW);
  Serial.println();
}


void loop() {

  int commsMotion = digitalRead(motionPin);
  int pirOut = 0;
  if (commsMotion == HIGH) {
    digitalWrite(ledPin, HIGH);
    commsMotion = 1;
  }
  datastreams[0].setInt(commsMotion);

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

      Serial.println(senderLongAddress.getLsb());
      uint32_t test = (senderLongAddress.getLsb());  

      // The option byte is a bit field
      if (rx.getOption() & ZB_PACKET_ACKNOWLEDGED)
        // the sender got an ACK
        Serial.println("packet acknowledged");
      if (rx.getOption() & ZB_BROADCAST_PACKET)
        // This was a broadcast packet
        Serial.println("broadcast Packet");

      Serial.print("checksum is ");
      Serial.println(rx.getChecksum(), HEX);

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
      // and an ascii representation for those of us
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
      Serial.println();
      // So, for example, you could do something like this:
      handleXbeeRxMessage(rx.getData(), rx.getDataLength());
      Serial.print("xbeereadstring:");
      Serial.println(xbeeReadString);

      if(test = 1084373003){ //Watermeter

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
        Serial2.flush();
        if(waterTimer2 == 60) { //Need to introduce a RTC to sync times with RL hours etc
          waterHourly = 0;
          waterTimer2 = 0;
        }
        if(waterTimer == 1440) { //resets Daily count after 1440 minutes (24 Hours)
          waterDaily = 0;
          waterTimer = 0;
        }
      }

      xbeeReadString = " ";
    }

    else if (xbee.getResponse().getApiId() == ZB_IO_SAMPLE_RESPONSE) {
      xbee.getResponse().getZBRxIoSampleResponse(ioSample);
      XBeeAddress64 senderLongAddress = ioSample.getRemoteAddress64();
      Serial.println(senderLongAddress.getLsb());
      uint32_t test = (senderLongAddress.getLsb());

      if (ioSample.containsAnalog()) {
        Serial.println("Sample contains analog data");
        Serial.print("Received I/O Sample from: ");
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
      if (test == 1083188734) {
        Serial.println("Xbee 1");
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
      }
      //      else if (test == 1081730785) {
      //        Serial.println("Xbee 2 - Not Processing!");
      //      }

      //this is else causes code to not upload properly.....????
      //      else if (test == 1082562186) {
      //        Serial.println("Xbee 3!!!!! - Not Processing!");
      //      }
    }
    else {
      Serial.print("Expected I/O Sample, but got ");
      Serial.print(xbee.getResponse().getApiId(), HEX);
    }    
  } 
  else if (xbee.getResponse().isError()) {
    Serial.print("Error reading packet.  Error code: ");  
    Serial.println(xbee.getResponse().getErrorCode());  
  }
  //  else {
  //*****************************************************
  //         Power receipt
  //*****************************************************
  //Serial.print("power....");

  if (timer >= 5000) {
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
    Serial1.flush();
    //  }
  }

  if (timer >= 5000) {

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
if(client.connect("emoncms.org",80)){
    Serial.print("Connecting.....");
    client.print("GET /input/post.json?json={");  // make sure there is a [space] between GET and /input
    client.print("CommsMotion:");
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
    client.print("}&apikey=");
    client.print(apiKey);         //assuming APIKEY is a char or string
    client.println(" HTTP/1.1");   //make sure there is a [space] BEFORE the HTTP
    client.println("Host: emoncms.org");
    client.println("User-Agent: Arduino-ethernet");
    client.println("Connection: close");     //    Although not technically necessary, I found this helpful
    client.println();


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
    Serial.print("}&apikey=");
    Serial.println(apiKey);         //assuming APIKEY is a char or string

    Serial.println(" HTTP/1.1");   //make sure there is a [space] BEFORE the HTTP
    Serial.println(F("Host: emoncms.org"));
    Serial.println(F("User-Agent: Arduino-ethernet"));
    Serial.println(F("Connection: close"));     //    Although not technically necessary, I found this helpful
    Serial.println();
    
    Serial.println("Upload to EmonCMS Completed");
    client.stop();
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














