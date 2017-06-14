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
#define DS1307_I2C_ADDRESS 0x68  // the I2C address of Tiny RTC
#define I2C24C32Add  0x50
#define BMP085_ADDRESS 0x77

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
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

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
// XivelyClient xivelyclient(client); ex cosm

int sensorPin = 0; //light
const int motionPin = 2; // choose the input pin (for PIR sensor)
const int ledPin = 3; //LED
const int foyeurLedPin = 7; //Foyeur Motion ON LED
int timer = 0;
const int hydroLED = 6; //LED that comes on with hotwater/heatpump

float strWater = 0;
float strBatteryV = 0;
float waterHourly = 0;
float waterDaily = 0;

char server[] = "http://emoncms.org/";     //emoncms URL
String apiKey = "ebd4f194e60f6e8694f56aa48b094ddb";

//powerserial
const int fNumber = 3; //number of fields to recieve in data stream
int fieldIndex = 0; //current field being recieved
int values[fNumber]; //array holding values

String xbeeReadString = "";
String xbeeReadString2 = "";

String realPower1;
String realPower2;
String realPower3;
String realPower4;
float realPower5 = 0;
float fltPower5 = 0;
String Irms1;
String Irms2;
String Irms3;
String Irms4;
String Vrms;
float KWHour = 0;
float KWHour2 = 0;
float KWDay = 0;
float lastKWHour = 0;
float lastKWHour2 = 0;
float lastKWDay = 0;
float minuteWattTotal = 0;
float hourWattTotal = 0;
float KWHourTotal = 0;
int kwStart = 0;
int firstStart = 0;

String foyeurLux;
String hotWaterHot;
String hotWaterCold;
String foyeurHumidity;
String foyeurTemp;
String FoyeurMotion;
float bathroomTemp = 0;
float bathroomVolt = 0;
float livingTemp = 0;
float livingVolt = 0;
float shedTemp = 0;

String strWeather;
String winDir;
String windSpeedkph;
String windGustkph;
String windGustDir;
String windSpdkph_avg2m;
String windDir_avg2m;
String windGustkph_10m;
String windGustDir_10m;
String humidity;
String WeatherSTemp;
String rainIn;
String dailyRainIn;
String pressure;
String batt_lvl;
String light_lvl;

int packetSize = xbeeReadString.length();
int currentCostMinute = 0;
int processMinute = 0;
int processHour = 0;
int processDay = 0;

int debug = 0;

//***************************************************
void setup() {
  Serial.begin(19200);  //Debug
  Serial1.begin(9600); //Currentcost chat
  Serial2.begin(9600); //Xbee chat
  Serial3.begin(57600); //Output to pi

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
    int processMinute = minute();
    int processHour = hour();
    int processDay = day();

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
  if (timeStatus() != timeSet)
    Serial.println("Unable to sync with the RTC");
  else
    Serial.println("RTC has set the system time");
  digitalClockDisplay();

  Serial.println("Starting Loop.....");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  setSyncProvider(RTC.get);   // the function to get the time from the RTC

  int commsMotion = digitalRead(motionPin);
  int pirOut = 0;
  if (commsMotion == HIGH) {
    digitalWrite(ledPin, HIGH);
    commsMotion = 1;
  }
  datastreams[0].setInt(commsMotion);
  /*
          if(minute() == 59 && second() == 59) { //Watertimer resets
            waterHourly = 0;
          }
          if(hour() == 23 && minute() == 59 && second() == 59) {
            waterDaily = 0;
          }
  */

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
      XBeeAddress64 senderLongAddress = rx.getRemoteAddress64();
      Serial.print("Got an rx packet from: ");


      print32Bits(senderLongAddress.getMsb());
      Serial.print(" ");
      print32Bits(senderLongAddress.getLsb());
      // this is how to get the sender's
      // 16 bit address and show it
      uint16_t senderShortAddress = rx.getRemoteAddress16();
      Serial.print(" (");
      print16Bits(senderShortAddress);
      Serial.println(")");
      Serial.print(senderLongAddress.getLsb());


      uint32_t xbee = (senderLongAddress.getLsb());
      // The option byte is a bit field
      if (rx.getOption() & ZB_PACKET_ACKNOWLEDGED)
        // the sender got an ACK
        Serial.println(" Packet Acknowledged");
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
      Serial.println(rx.getDataLength(), DEC);

      // this is the actual data you sent
      // Serial.println("Received Data: ");
      //for (int i = 0; i < rx.getDataLength(); i++) {
      //  print8Bits(rx.getData()[i]);
      //  Serial.print(" ");
      //}

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

      if (xbee == 1084373003) { //Watermeter
        Serial.println("=========Water Meter=========");
        String water = xbeeReadString.substring(17, 22);
        String batteryV = xbeeReadString.substring(23, 30);

        water.trim();
        batteryV.trim();
        Serial.println(water);
        Serial.println(batteryV);
        char floatbuf[8]; // make this at least big enough for the whole string
        water.toCharArray(floatbuf, sizeof(floatbuf));
        strWater = atof(floatbuf);
        batteryV.toCharArray(floatbuf, sizeof(floatbuf));
        strBatteryV = atof(floatbuf) * (3.3 / 1023);

        waterHourly = waterHourly + strWater;
        waterDaily = waterDaily + strWater;
        datastreams[12].setFloat(strWater);
        datastreams[13].setFloat(waterHourly);
        datastreams[14].setFloat(waterDaily);
        Serial.print("Battery Voltage:");
        Serial.print(strBatteryV);
        Serial.println("V");
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

      }

      if (xbee == 1085127839 && packetSize > 60) { //Weather Station
        Serial.println("=========Weather Station 1=========");
        String xbeeReadString2 = xbeeReadString.substring(17, 100);
        String batteryV = xbeeReadString.substring(84, 89);
        //        xbeeReadString2.trim();
        batteryV.trim();
        Serial.println(xbeeReadString2);
        Serial.println(batteryV);
        char floatbuf[30]; // make this at least big enough for the whole string
        //    xbeeReadString2.toCharArray(floatbuf, sizeof(floatbuf));
        //strWeather = atof(floatbuf);
        batteryV.toCharArray(floatbuf, sizeof(floatbuf));
        strBatteryV = atof(floatbuf) * (3.3 / 1023);

        winDir = xbeeReadString2.substring(0, 3);
        windSpeedkph = xbeeReadString2.substring(4, 9);
        windGustkph = xbeeReadString2.substring(10, 15);
        windGustDir = xbeeReadString2.substring(16, 19);
        windSpdkph_avg2m = xbeeReadString2.substring(20, 25);
        windDir_avg2m = xbeeReadString2.substring(26, 29);
        windGustkph_10m = xbeeReadString2.substring(30, 35);
        windGustDir_10m = xbeeReadString2.substring(36, 39);
        humidity = xbeeReadString2.substring(40, 45);
        WeatherSTemp = xbeeReadString2.substring(46, 51);
        rainIn = xbeeReadString2.substring(52, 56);
        dailyRainIn = xbeeReadString2.substring(57, 64);
        /*
          pressure = xbeeReadString2.substring(65, 71);
          batt_lvl = xbeeReadString2.substring(72, 77);
          light_lvl = xbeeReadString2.substring(78, 83);
        */

        winDir.trim();
        windSpeedkph.trim();
        windGustkph.trim();
        windGustDir.trim();
        windSpdkph_avg2m.trim();
        windDir_avg2m.trim();
        windGustkph_10m.trim();
        windGustDir_10m.trim();
        humidity.trim();
        WeatherSTemp.trim();
        rainIn.trim();
        dailyRainIn.trim();
        /*
          pressure.trim();
          batt_lvl.trim();
          light_lvl.trim();
        */

        Serial.print("winddir=");
        Serial.print(winDir);
        Serial.print(",windSpeedmph=");
        Serial.print(windSpeedkph);
        Serial.print(",windGustmph=");
        Serial.print(windGustkph);
        Serial.print(",windGustdir=");
        Serial.print(windGustDir);
        Serial.print(",windSpdmph_avg2m=");
        Serial.print(windSpdkph_avg2m);
        Serial.print(",windDir_avg2m=");
        Serial.print(windDir_avg2m);
        Serial.print(",windGustmph_10m=");
        Serial.print(windGustkph_10m);
        Serial.print(",windGustDir_10m=");
        Serial.print(windGustDir_10m);
        Serial.print(",humidity=");
        Serial.print(humidity);
        Serial.print(",WeatherSTemp=");
        Serial.print(WeatherSTemp);
        Serial.print(",rainin=");
        Serial.print(rainIn);
        Serial.print(",dailyrainin=");
        Serial.print(dailyRainIn);
        /*
          Serial.print(",pressure=");
          Serial.print(pressure);
          Serial.print(",batt_lvl=");
          Serial.print(batt_lvl);
          Serial.print(",light_lvl=");
          Serial.print(light_lvl);
        */
        Serial.print(",XBee V=");
        Serial.print(strBatteryV);
        Serial.println("===========================");
        Serial2.flush();
        xbeeReadString = "";
        xbeeReadString2 = "";
      }

      if (xbee == 1085127839 && packetSize < 60) { //Weather Station
      Serial.println("=========Weather Station 2=========");
        String xbeeReadString2 = xbeeReadString.substring(17, 36);
        Serial.println(xbeeReadString2);
        char floatbuf[30]; // make this at least big enough for the whole string

        pressure = xbeeReadString2.substring(0, 6);
        batt_lvl = xbeeReadString2.substring(7, 12);
        light_lvl = xbeeReadString2.substring(13, 18);

        pressure.trim();
        batt_lvl.trim();
        light_lvl.trim();

        Serial.print("pressure=");
        Serial.print(pressure);
        Serial.print(",batt_lvl=");
        Serial.print(batt_lvl);
        Serial.print(",light_lvl=");
        Serial.print(light_lvl);
        Serial.println("===========================");

        Serial2.flush();
        xbeeReadString = "";
        xbeeReadString2 = "";

      }
      
      if (xbee == 1081730785 && packetSize > 40) { //Foyeur
      Serial.println("=========Foyeur=========");
        String xbeeReadString2 = xbeeReadString.substring(17, 51);
        if (debug == 1) {
          Serial.print("String1=");
          Serial.println(xbeeReadString);
          Serial.print("String2=");
          Serial.println(xbeeReadString2); //String2=57.25,18.00,18.00,58.20,18.20,0
        }
        foyeurLux = xbeeReadString2.substring(0, 7);
        hotWaterHot = xbeeReadString2.substring(8, 13);
        hotWaterCold = xbeeReadString2.substring(14, 19);
        foyeurHumidity = xbeeReadString2.substring(20, 25);
        foyeurTemp = xbeeReadString2.substring(26, 31);
        FoyeurMotion = xbeeReadString2.substring(32, 33);

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

      if (xbee == 1081730785 && packetSize < 35) {
      Serial.println("=========Foyeur=========");
        String xbeeReadString2 = xbeeReadString.substring(17, 32);
        if (debug == 1) {
          Serial.print("String1=");
          Serial.println(xbeeReadString);
          Serial.print("String2=");
          Serial.println(xbeeReadString2); //String2=57.25,18.00,18.00,58.20,18.20,0õ
        }
        String FoyeurMotion = xbeeReadString2.substring(0, 1);
        Serial.print("Foyeur Motion: ");
        Serial.println(FoyeurMotion);
        xbeeReadString = "";
        xbeeReadString2 = "";

        char floatbuf[10]; // make this at least big enough for the whole string
        FoyeurMotion.toCharArray(floatbuf, sizeof(floatbuf));
        float motion = atof(floatbuf);
        if (motion = 1.00) {
          digitalWrite(foyeurLedPin, HIGH);
        }
      }


      if (xbee == 1081730797 && packetSize > 20) { //powermeter
      Serial.print("=========Power Meter=========");
        String xbeeReadString2 = xbeeReadString.substring(17, 83);
        if (debug == 1) {
          Serial.print("Packet Size: ");
          Serial.println(packetSize, DEC);
          Serial.print("String1=");
          Serial.println(xbeeReadString);
          Serial.print("String2=");
          Serial.println(xbeeReadString2);
        }

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

        //Replaced with trim function above. Stoll needed for realpower5
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

        minuteWattTotal += fltPower4;
        Serial.println();
        Serial.print("Watt Total:"); Serial.print(minuteWattTotal);
        Serial.print("---KW/H:"); Serial.print(KWHour / 1000);
        Serial.print("---KW/H2:"); Serial.print(KWHour2 / 1000);
        Serial.print("---Last KW/H:"); Serial.print(lastKWHour / 1000);
        Serial.print("---Last KW/H:"); Serial.print(lastKWHour2 / 1000);
        Serial.print("---KW/D:"); Serial.println(KWDay / 1000);

        Serial.println("************Power stats sent to python***********");
        Serial3.print("{");
        Serial3.print("TotalPowerWatts:"); Serial3.print(realPower4); Serial3.print(",");
        Serial3.print("SolarWatts:"); Serial3.print(realPower1); Serial3.print(",");
        Serial3.print("SpareWatts:"); Serial3.print(realPower2); Serial3.print(",");
        Serial3.print("HotWaterHeaterWatts:"); Serial3.print(realPower3); Serial3.print(",");
        Serial3.print("PowerpointsLights:"); Serial3.print(fltPower5); Serial3.print(",");
        Serial3.print("TotalCurrent:"); Serial3.print(Irms4); Serial3.print(",");
        Serial3.print("SolarCurrent:"); Serial3.print(Irms1); Serial3.print(",");
        Serial3.print("SpareCurrent:"); Serial3.print(Irms2); Serial3.print(",");
        Serial3.print("hotwaterHeaterCurrent:"); Serial3.print(Irms3); Serial3.print(",");
        Serial3.print("LineVoltage:"); Serial3.println(Vrms);
        Serial3.print("}");

        Serial.print("CT1 Solar:");
        Serial.print(realPower1);
        //      Serial.print("  CT2 Spare: "); //not hooked up
        //      Serial.print(realPower2);
        Serial.print("  CT3 Hydro: ");
        Serial.print(realPower3);
        Serial.print("  CT4 Total: ");
        Serial.print(realPower4);
        Serial.print("  PP/Lights: ");
        Serial.println(fltPower5);
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
        Serial.println(" ");
        if (fltPower3 > 40) {
          digitalWrite(hydroLED, HIGH);
        }
        else {
          digitalWrite(hydroLED, LOW);
        }

        Serial2.flush();
        xbeeReadString = "";
        xbeeReadString2 = "";

      }


    }
    // XBEE IO Samples
    else if (xbee.getResponse().getApiId() == ZB_IO_SAMPLE_RESPONSE) {
      xbee.getResponse().getZBRxIoSampleResponse(ioSample);
      XBeeAddress64 senderLongAddress = ioSample.getRemoteAddress64();
      //      Serial.println(senderLongAddress.getLsb()); //moved down
      uint32_t xbee = (senderLongAddress.getLsb());

      if (ioSample.containsAnalog()) {
        // Serial.println("Sample contains analog data");
        Serial.print("Received I/O Sample from: ");
        Serial.println(senderLongAddress.getLsb());
        // this is how you get the 64 bit address out of
        // the incoming packet so you know which device
        // it came from
        uint8_t bitmask = ioSample.getAnalogMask();

        if (debug == 1) {
          for (uint8_t x = 0; x < 8; x++) {
            if ((bitmask & (1 << x)) != 0) {
              Serial.print("position ");
              Serial.print(x, DEC);
              Serial.print(" value: ");
              Serial.print(ioSample.getAnalog(x));
              Serial.println();
            }
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

       if (xbee == 1097062709) {
        Serial.println("==========Shed==========");
        int reading = (ioSample.getAnalog(0));
        Serial.println(reading);
        float voltage = reading * 1.2;
        voltage /= 1024.0;
        Serial.println(voltage);
        float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
        //to degrees ((volatge - 500mV) times 100)
        Serial.print(temperatureC);
        Serial.println(" degrees C");
        shedTemp = temperatureC;

        int vReading3 = (ioSample.getAnalog(7));
        float xbee1v = vReading3 * 1.2 / 1024;
        // voltage /= 1024.0;
        Serial.print(xbee1v);
        Serial.println(" Shed Xbee Voltage Not logged");
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
        int vReading3 = (ioSample.getAnalog(7));
        float xbee1v = vReading3 * 1.2 / 1024;
        Serial.print(xbee1v);
        Serial.println(" Xbee Voltage Not logged");
        Serial2.flush();
        Serial.println("===========================");
        datastreams[16].setFloat(temperatureC);
      }

      if (xbee == 1085374409) {
        Serial.println("==========Living Room==========");
        int reading = (ioSample.getAnalog(0));
        float voltage = reading * 1.2;
        voltage /= 1024.0;
        livingTemp = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
        //to degrees ((volatge - 500mV) times 100)
        Serial.print(livingTemp);
        Serial.println(" degrees C");
        int vReading4 = (ioSample.getAnalog(7));
        livingVolt = vReading4 * 1.2 / 1024;
        // voltage /= 1024.0;
        Serial.print(livingVolt);
        Serial.println(" Xbee Voltage Not logged");
        Serial.println("===========================");
        Serial2.flush();
      }
      if (xbee == 1085233956) {
        Serial.println("==========Bathroom==========");
        int reading = (ioSample.getAnalog(0));
        float voltage = reading * 1.2;
        voltage /= 1024.0;
        bathroomTemp = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
        //to degrees ((volatge - 500mV) times 100)
        Serial.print(bathroomTemp);
        Serial.println(" degrees C");

        int vReading3 = (ioSample.getAnalog(7));
        bathroomVolt = vReading3 * 1.2 / 1024;
        // voltage /= 1024.0;
        Serial.print(bathroomVolt);
        Serial.println(" Xbee Voltage Not logged");
        Serial.println("===========================");
        Serial2.flush();
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



  /* Currentcost removed. Replaced by power Ardy
    //*****************************************************
    //         Power receipt
    //*****************************************************
    if (currentCostMinute != minute()) {
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
      currentCostMinute = minute();
    }
  */
  if (processMinute != minute()) {
    digitalClockDisplay();
    if (firstStart == 0) {
      kwStart = minute();
      hourWattTotal += minuteWattTotal;
      KWHourTotal += (minuteWattTotal / 6);
      KWHour = KWHourTotal / ((minute() + 1) - kwStart);
      KWHour2 = hourWattTotal / ((minute() + 1) - kwStart / 6);
      minuteWattTotal = 0;
      firstStart = 1;
    }
    else if (firstStart == 1) {
      hourWattTotal += minuteWattTotal;
      KWHourTotal += (minuteWattTotal / 6);
      KWHour = KWHourTotal / ((minute() + 1) - kwStart);
      KWHour2 = hourWattTotal / ((minute() + 1) - kwStart / 6);
      minuteWattTotal = 0;
    }
    else if (firstStart == 2) {
      hourWattTotal += minuteWattTotal;
      KWHourTotal += (minuteWattTotal / 6);
      KWHour = KWHourTotal / (minute() + 1);
      KWHour2 = hourWattTotal / ((minute() + 1) / 6);
      minuteWattTotal = 0;
    }

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

    /* Leftovers? remarked out 29/04/2015
        Serial.print("lightsandpowah: ");
        Serial.println(values[2]);
        Serial.print("hydro: ");
        Serial.println(values[1]);
        Serial.print("totalpower: ");
        Serial.println(values[0]);
    */
    //SQL feed
    Serial.println("SQL:");
    //Serial 3 to Pi
    //converting to json 29/04/2015
    Serial3.print("{");
    Serial3.print("CommsMotion:"); Serial3.print(datastreams[0].getInt()); Serial3.print(",");
    Serial3.print("CommsTemp:"); Serial3.print(datastreams[1].getFloat()); Serial3.print(",");
    Serial3.print("CommsBarometer:"); Serial3.print(datastreams[2].getFloat()); Serial3.print(",");
    Serial3.print("CommsHumidity:"); Serial3.print(datastreams[3].getFloat()); Serial3.print(",");
    Serial3.print("CommsLux:"); Serial3.print(datastreams[4].getInt()); Serial3.print(",");
    Serial3.print("OutdoorTemp:"); Serial3.print(datastreams[5].getFloat()); Serial3.print(",");
    Serial3.print("OutdoorV:"); Serial3.print(datastreams[6].getFloat()); Serial3.print(",");
    Serial3.print("OutdoorBatteryV:"); Serial3.print(datastreams[7].getFloat()); Serial3.print(",");
    Serial3.print("OutsideSolarV:"); Serial3.print(datastreams[8].getFloat()); Serial3.print(",");
    //Serial3.print("CommsMotion:");Serial3.print(datastreams[9].getInt());Serial3.print(",");
    //Serial3.print("CommsMotion:");Serial3.print(datastreams[10].getInt());Serial3.print(",");
    Serial3.print("WaterBattery:"); Serial3.print(strBatteryV); Serial3.print(",");
    Serial3.print("WatterUsage:"); Serial3.print(datastreams[12].getFloat()); Serial3.print(",");
    Serial3.print("WaterusageHourly:"); Serial3.print(datastreams[13].getFloat()); Serial3.print(",");
    Serial3.print("WaterusageDaily:"); Serial3.print(datastreams[14].getFloat()); Serial3.print(",");
    Serial3.print("Bedroom1Temp:"); Serial3.print(datastreams[15].getFloat()); Serial3.print(",");
    Serial3.print("LaundryTemp:"); Serial3.print(datastreams[16].getFloat()); Serial3.print(",");
    Serial3.print("FoyeurLux:"); Serial3.print(foyeurLux); Serial3.print(",");
    Serial3.print("HotwaterHotOutTemp:"); Serial3.print(hotWaterHot); Serial3.print(",");
    Serial3.print("HotwaterColdInTemp:"); Serial3.print(hotWaterCold); Serial3.print(",");
    Serial3.print("FoyeurHumidity:"); Serial3.print(foyeurHumidity); Serial3.print(",");
    Serial3.print("FoyeurTemp:"); Serial3.print(foyeurTemp); Serial3.print(",");
    Serial3.print("Bathroomtemp:"); Serial3.print(bathroomTemp); Serial3.print(",");
    Serial3.print("LivingTemp:"); Serial3.print(livingTemp); Serial3.print(",");
    Serial3.print("ShedTemp:"); Serial3.print(shedTemp); Serial3.print(",");
    Serial3.print("FoyeurMotion:"); Serial3.print(FoyeurMotion);Serial3.print("}");

    //debug console
    if (debug > 0 ) {
      Serial.print("CommsMotion:"); Serial.print(datastreams[0].getInt()); Serial.print(",");
      Serial.print("CommsTemp:"); Serial.print(datastreams[1].getFloat()); Serial.print(",");
      Serial.print("CommsBarometer:"); Serial.print(datastreams[2].getFloat()); Serial.print(",");
      Serial.print("CommsHumidity:"); Serial.print(datastreams[3].getFloat()); Serial.print(",");
      Serial.print("CommsLux:"); Serial.print(datastreams[4].getInt()); Serial.print(",");
      Serial.print("OutdoorTemp:"); Serial.print(datastreams[5].getFloat()); Serial.print(",");
      Serial.print("OutdoorV:"); Serial.print(datastreams[6].getFloat()); Serial.print(",");
      Serial.print("OutdoorBatteryV:"); Serial.print(datastreams[7].getFloat()); Serial.print(",");
      Serial.print("OutsideSolarV:"); Serial.print(datastreams[8].getFloat()); Serial.print(",");
      //Serial.print("CommsMotion:");Serial.print(datastreams[9].getInt());Serial.print(",");
      //Serial.print("CommsMotion:");Serial.print(datastreams[10].getInt());Serial.print(",");
      Serial.print("WaterBattery:"); Serial.print(strBatteryV); Serial.print(",");
      Serial.print("WatterUsage:"); Serial.print(datastreams[12].getFloat()); Serial.print(",");
      Serial.print("WaterusageHourly:"); Serial.print(datastreams[13].getFloat()); Serial.print(",");
      Serial.print("WaterusageDaily:"); Serial.print(datastreams[14].getFloat()); Serial.print(",");
      Serial.print("Bedroom1Temp:"); Serial.print(datastreams[15].getFloat()); Serial.print(",");
      Serial.print("LaundryTemp:"); Serial.print(datastreams[16].getFloat()); Serial.print(",");
      Serial.print("FoyeurLux:"); Serial.print(foyeurLux); Serial.print(",");
      Serial.print("HotwaterHotOutTemp:"); Serial.print(hotWaterHot); Serial.print(",");
      Serial.print("HotwaterColdInTemp:"); Serial.print(hotWaterCold); Serial.print(",");
      Serial.print("FoyeurHumidity:"); Serial.print(foyeurHumidity); Serial.print(",");
      Serial.print("FoyeurTemp:"); Serial.print(foyeurTemp); Serial.print(",");
      Serial.print("FoyeurMotion:"); Serial.print(FoyeurMotion); Serial.print(",");
      Serial.print("TotalPowerWatts:"); Serial.print(realPower4); Serial.print(",");
      Serial.print("SolarWatts:"); Serial.print(realPower1); Serial.print(",");
      Serial.print("SpareWatts:"); Serial.print(realPower2); Serial.print(",");
      Serial.print("HotWater&Heater:"); Serial.print(realPower3); Serial.print(",");
      Serial.print("Powerpoints&Lights:"); Serial.print(fltPower5); Serial.print(",");
      Serial.print("TotalCurrent:"); Serial.print(Irms4); Serial.print(",");
      Serial.print("SolarCurrent:"); Serial.print(Irms1); Serial.print(",");
      Serial.print("SpareCurrent:"); Serial.print(Irms2); Serial.print(",");
      Serial.print("hotwater&Heater:"); Serial.print(Irms3); Serial.print(",");
      Serial.print("LineVoltage:"); Serial.println(Vrms);
    }
    Serial.println("********SQL Injected!*********");
    Serial.println();

    /*
        Serial.println("Uploading it to Xively");
        int ret = xivelyclient.put(feed, xivelyKey);
        Serial.print("xivelyclient.put returned ");
        Serial.println(ret);
    */

    //reset comms motion switch here to update interval for motion detected not just to update if commsmotion and activity update coincides like old way
    digitalWrite(ledPin, LOW);
    digitalWrite(foyeurLedPin, LOW);
    commsMotion = 0;
    timer = 0;
    Serial.println();
    processMinute = minute();
    //   processDay = day();
  }

  if (processHour != hour()) {
    hourWattTotal += minuteWattTotal;
    processHour = hour();
    hourWattTotal / (minute() * 6);
    KWHourTotal = 0;
    KWDay += KWHour;
    lastKWHour = KWHour;
    lastKWHour2 = KWHour2;
    KWHour = 0;
    KWHour2 = 0;
    kwStart = 2;
    waterHourly = 0;
    Serial.println(")************************");
    digitalClockDisplay();
    Serial.println(")************************");
  }

  if (processDay != day()) {
    processDay = day();
    waterDaily = 0 ;
  }
}


void handleXbeeRxMessage(uint8_t *data, uint8_t length) {
  // this is just a stub to show how to get the data,
  // and is where you put your code to do something with
  // it.
  for (int i = 0; i < length; i++) {
    //  char try[80];
    char xbuff = data[i];
    xbeeReadString += xbuff;
    //   Serial.print(final,DEC);
    // Serial.print(data[i]);
  }
  Serial.println();
}

void showFrameData() {
  Serial.println("Incoming frame data:");
  for (int i = 0; i < xbee.getResponse().getFrameDataLength(); i++) {
    print8Bits(xbee.getResponse().getFrameData()[i]);
    Serial.print(' ');
  }
  Serial.println();
  for (int i = 0; i < xbee.getResponse().getFrameDataLength(); i++) {
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
void print32Bits(uint32_t dw) {
  print16Bits(dw >> 16);
  print16Bits(dw & 0xFFFF);
}

void print16Bits(uint16_t w) {
  print8Bits(w >> 8);
  print8Bits(w & 0x00FF);
}

void print8Bits(byte c) {
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

void digitalClockDisplay() {
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

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
