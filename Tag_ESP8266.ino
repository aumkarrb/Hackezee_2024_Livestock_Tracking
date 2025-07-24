#include <ESP8266WiFi.h>
#include <espnow.h>
#include <MQUnifiedsensor.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_MPU6050.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

#define DHTPIN 2 
#define DHTTYPE DHT11 
#define Board "ESP8266"
#define Pin A0
#define Type "MQ-135"
#define Voltage_Resolution 3.3
#define ADC_Bit_Resolution 10
#define RatioMQ135CleanAir 3.6
#define RX 12
#define TX 14
#define GPS_BAUD 9600

DHT_Unified dht(DHTPIN, DHTTYPE);
MQUnifiedsensor MQ135(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin, Type);
Adafruit_MPU6050 mpu;
TinyGPSPlus gps;
SoftwareSerial gpsSerial(RX, TX);

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message {
    int ID;
    double GPS_N;
    double GPS_W;
    float Temperature;
    float Humidity;
    float Acc_x;
    float Acc_y;
    float Acc_z;
    float Air_Quality;
    float Battery;
} struct_message;

struct_message myData;

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  if (sendStatus == 0)
    Serial.println("Delivery success");
  else
    Serial.println("Delivery fail");
}
 
void setup() {
  Serial.begin(115200);

  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);

  MQ135.setRegressionMethod(1); 
  MQ135.setA(162.453); MQ135.setB(-3.211);
  MQ135.init();
  /*
  Exponential regression:
  GAS      | a      | b
  CO       | 605.18 | -3.937  
  Alcohol  | 77.255 | -3.18 
  CO2      | 110.47 | -2.862
  Toluen  | 44.947 | -3.445
  NH4      | 102.2  | -2.473
  Aceton  | 34.668 | -3.369
  */

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

  gpsSerial.begin(GPS_BAUD);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);

  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}
 
void loop() {
  sensors_event_t event;
  float Rs_t;
  float R0_t;
  sensors_event_t a, g, temp;

  myData.ID = 1;

  dht.temperature().getEvent(&event);
  if (isnan(event.temperature))
    Serial.println(F("Error reading temperature!"));
  else 
    myData.Temperature = event.temperature;

  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity))
    Serial.println(F("Error reading humidity!"));
  else
    myData.Humidity = event.relative_humidity;

  MQ135.update();
  MQ135.readSensor();
  Rs_t = MQ135.getRS();
  R0_t = 3.6297;
  myData.Air_Quality = Rs_t/R0_t;

  mpu.getEvent(&a, &g, &temp);
  myData.Acc_x = a.acceleration.x;
  myData.Acc_y = a.acceleration.y;
  myData.Acc_z = a.acceleration.z;

  if (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
      if (gps.location.isUpdated()) {
          myData.GPS_N = gps.location.lat();
          myData.GPS_W = gps.location.lng();
      }
  }
    
  myData.Battery = random(0,50);

    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  delay(2000);
}