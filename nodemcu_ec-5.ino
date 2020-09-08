#include "Wire.h"

#define RTC_ENABLED         0
#define EC5_ENABLED         1

#if EC5_ENABLED
  #define EC5_PWR_PIN      16 //This is GPIO16(D0)
  #define EC5_INPUT        A0
  /**
   * @brief Function reads raw voltage reading.
   */
  float ec5VoltageReading();
  /**
   * @brief Function converts rar voltage reading to volumetric water content(VWC) reading.
   */
  void ec5VWCReading();
  /**
   * @brief Function sends updates to Thingspeak
   */
  void sendUpdate();
#endif// EC5_ENABLED
#if RTC_ENABLED
  #define SET_RTC_TIME_ENABLED  0
  #define DS3231_I2C_ADDRESS    0x68
  // Convert normal decimal numbers to binary coded decimal
  byte decToBcd(int val)
  {
    return ( (val / 10 * 16) + (val % 10) );
  }
  // Convert binary coded decimal to normal decimal numbers
  int bcdToDec(int val)
  {
    return ( (val / 16 * 10) + (val % 16) );
  }
  void displayTime();
  void readDS3231time(int *second,int *minute,int *hour,int *dayOfWeek,int *day,int *month,int *year);
  #if SET_RTC_TIME_ENABLED
      void setDS3231time(int second, int minute, int hour, int dayOfWeek, int dayOfMonth, int month, int year);
  #endif //SET_RTC_TIME_ENABLED
#endif //

void setup()
{
  Serial.begin(115200);
  Wire.begin(4,0);
  Wire.setClock(400000L);   // set I2C clock to 400kHz
  #if SET_RTC_TIME_ENABLED
    /*setDS3231time(int second, int minute, int hour, int dayOfWeek, int dayOfMonth, int month, int year(0-99))*/
    setDS3231time(28, 16, 16, 3, 8, 9, 20);
  #endif // SET_RTC_TIME_ENABLED
  #if EC5_ENABLED
    pinMode(EC5_PWR_PIN, OUTPUT);
    pinMode(EC5_INPUT, INPUT);
  #endif// EC5_ENABLED
}

void loop()
{
  #if EC5_ENABLED
      ec5VoltageReading();
      #if RTC_ENABLED
          sendUpdate();
      #endif //RTC_ENABLED
  #endif //EC5_ENABLED
  #if RTC_ENABLED
    displayTime();
  #endif //RTC_ENABLED
  delay(2000);
}

#if EC5_ENABLED
  void ec5VWCReading()
  {
    float avg = ec5VoltageReading();  
    /*      
     * Using a 10 bit ADC, the formula used is:
     *      VWC = 0.0014*(ADC output) - 0.4697
     *      link: https://www.researchgate.net/publication/320668407_An_Arduino-Based_Wireless_Sensor_Network_for_Soil_Moisture_Monitoring_Using_Decagon_EC-5_Sensors
     */
    float vwcValue = 0.0014 * avg - 0.4697;
    Serial.print("VWC Value: "); Serial.println(vwcValue);
  }
  float ec5VoltageReading()
  {
    digitalWrite(EC5_PWR_PIN, HIGH);
    int array[10], sum=0;
    float avg;
    for(int i=0; i<10;i++)
    {
        delayMicroseconds(15000);
        array[i] = analogRead(EC5_INPUT);    
    }
    digitalWrite(EC5_PWR_PIN, LOW);
    for(int x:array)
    {
     Serial.print("x: "); Serial.println(x);
     sum+=x;   
    }
    Serial.print("sum: "); Serial.println(sum);
    avg = (float)sum / 10;
    Serial.print("avg: "); Serial.println(avg); 
    return avg;   
  }
  #if RTC_ENABLED
     void sendUpdate() //Sends an update to the farmer after every x(30) minutes
     {
       static int count = 0;
       // Get data from the DS3231
       int second, minute, hour, dayOfWeek, day, month, year;
       // retrieve data from DS3231
       readDS3231time(&second, &minute, &hour, &dayOfWeek, &day, &month, &year);
       int rem = minute % 30;
       if (rem == 0 && count == 0) 
       {
         ec5VWCReading();
         count++;
       }
       else if (rem > 0)
       {
         count = 0;
       }
     }
   #endif// RTC_ENABLED
#endif// EC5_ENABLED

#if RTC_ENABLED
  #if SET_RTC_TIME_ENABLED
    void setDS3231time(int second, int minute, int hour, int dayOfWeek, int dayOfMonth, int month, int year)
    {
      // sets time and date data to DS3231
      Wire.beginTransmission(DS3231_I2C_ADDRESS);
      Wire.write(0); // set next input to start at the seconds register
      Wire.write(decToBcd(second)); // set seconds
      Wire.write(decToBcd(minute)); // set minutes
      Wire.write(decToBcd(hour)); // set hours
      Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
      Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
      Wire.write(decToBcd(month)); // set month
      Wire.write(decToBcd(year)); // set year (0 to 99)
      Wire.endTransmission();
    }
  #endif //SET_RTC_TIME_ENABLED
  void readDS3231time(int *second,int *minute,int *hour,int *dayOfWeek,int *day,int *month,int *year)
  {
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0); // set DS3231 register pointer to 00h
    Wire.endTransmission();
    Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
    // request seven bytes of data from DS3231 starting from register 00h
    *second = bcdToDec(Wire.read() & 0x7f);
    *minute = bcdToDec(Wire.read());
    *hour = bcdToDec(Wire.read() & 0x3f);
    *dayOfWeek = bcdToDec(Wire.read());
    *day = bcdToDec(Wire.read());
    *month = bcdToDec(Wire.read());
    *year = bcdToDec(Wire.read());
  }
  void displayTime()
  {
    int second, minute, hour, dayOfWeek, day, month, year;
    // retrieve data from DS3231
    readDS3231time(&second, &minute, &hour, &dayOfWeek, &day, &month, &year);
    // send it to the serial monitor
    Serial.print(hour);
    // convert the byte variable to a decimal number when displayed
    Serial.print(":");
    if (minute < 10)
    {
      Serial.print("0");
    }
    Serial.print(minute);
    Serial.print(":");
    if (second < 10)
    {
      Serial.print("0");
    }
    Serial.print(second);
    Serial.print(" ");
    Serial.print(day);
    Serial.print("/");
    Serial.print(month);
    Serial.print("/");
    Serial.print(year);
    Serial.print(" Day of week: ");
    switch (dayOfWeek) {
      case 1:
        Serial.println("Sunday");
        break;
      case 2:
        Serial.println("Monday");
        break;
      case 3:
        Serial.println("Tuesday");
        break;
      case 4:
        Serial.println("Wednesday");
        break;
      case 5:
        Serial.println("Thursday");
        break;
      case 6:
        Serial.println("Friday");
        break;
      case 7:
        Serial.println("Saturday");
        break;
      default:
        Serial.println(dayOfWeek);
        break;
    }
  }
#endif //RTC_ENABLED
