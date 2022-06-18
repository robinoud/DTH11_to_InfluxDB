#include <WiFi.h>
#include <esp_wpa2.h>
#include <esp_wifi.h>
#include <time.h>
#include <InfluxDbClient.h>   // https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino
#include <Wire.h>
#include <Adafruit_BME280.h>  // https://github.com/adafruit/Adafruit_BME280_Library

#define HOSTNAME "ESP32 Temperature Sensor"
#define LOCATION "1000 chem"

//Check instructions at https://github.com/debsahu/Esp32_EduWiFi to connect to WPA2-Enterprise WiFi
//Register your device using MAC address in MSetup
const char* ssid = "Vanidear_2.4G";
const char* password = "sirinart8611"; 


static const char incommon_ca[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
-----END CERTIFICATE-----
)EOF";

// Obtain the latest from :
// https://documentation.its.umich.edu/content/wifi-manually-configuring-wpa2-enterprise-other-wifi-enabled-devices-unsupported-devices
// Use the "Intermediate CA" file: http://www.itcom.itd.umich.edu/downloads/wifi/incommon_ras_server_ca.cer ^^^^^^^^^^^^^^^^^^^^^^^^^^^
// NO need for "Root CA: UserTrust RSA Cerification Authority" file

// InfluxDB  server url. Don't use localhost, always server name or ip address.
// E.g. http://192.168.1.48:8086 (In InfluxDB 2 UI -> Load Data -> Client Libraries),
#define INFLUXDB_URL "http://127.0.0.1:8086"
// InfluxDB 2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "FtSvsANkRzE0v-nB3X7Nh_6DUWEDe6PHpsVHAViy7pswLekIiPxMebTsR9kuaO9PCIn5JpTyx-SPBC4Gw8RmZQ=="
// InfluxDB 2 organization id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
#define INFLUXDB_ORG "org"
// InfluxDB 2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
#define INFLUXDB_BUCKET "sensors"

// InfluxDB client instance
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

// Data point
Point sensor("room_sensor");

const char *ntpServer = "pool.ntp.org";
const char *timezoneEST = "EST5EDT,M3.2.0/2,M11.1.0";

uint8_t counter = 0;
unsigned long previousMillisWiFi = 0;
char timeStringBuff[50];
//uint8_t masterCustomMac[] = {0x24, 0x0A, 0xC4, 0x9A, 0xA5, 0xA1}; // 24:0a:c4:9a:a5:a1

Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();

void printLocalTime(bool printToSerial = false)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.print(F("NTP sync failed "));
        return;
    }
    strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    if (printToSerial)
        Serial.println(timeStringBuff);
}

void sendData()
{
    sensors_event_t temp_event, pressure_event, humidity_event;
    bme_temp->getEvent(&temp_event);
    bme_pressure->getEvent(&pressure_event);
    bme_humidity->getEvent(&humidity_event);

    // Store measured value into point
    sensor.clearFields();

    // Data
    sensor.addField("temperature", temp_event.temperature);
    sensor.addField("humidity", humidity_event.relative_humidity);
    sensor.addField("pressure", pressure_event.pressure);
    // Print what are we exactly writing
    Serial.print("Writing: ");
    Serial.println(client.pointToLineProtocol(sensor));

    // Write point
    if (!client.writePoint(sensor))
    {
        Serial.print("InfluxDB write failed: ");
        Serial.println(client.getLastErrorMessage());
    }
}

void setup()
{
    Serial.begin(115200);
    delay(10);
    Serial.println();

    if (!bme.begin())
    {
        Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
        while (1)
            delay(10);
    }

    bme_temp->printSensorDetails();
    bme_pressure->printSensorDetails();
    bme_humidity->printSensorDetails();
    WiFi.disconnect(true); //disconnect form wifi to set new wifi connection
    WiFi.mode(WIFI_STA);   //init wifi mode
    // esp_wifi_set_mac(WIFI_IF_STA, &masterCustomMac[0]);
    Serial.print("MAC >> ");
    Serial.println(WiFi.macAddress());
    Serial.printf("Connecting to WiFi: %s ", ssid);
    WiFi.setHostname(HOSTNAME);
    esp_wifi_sta_wpa2_ent_set_ca_cert((uint8_t *)incommon_ca, strlen(incommon_ca) + 1);
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
    esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
    esp_wifi_sta_wpa2_ent_enable(&config);
    WiFi.begin(ssid);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(F("."));
        counter++;
        if (counter >= 60)
        { //after 30 seconds timeout - reset board
            ESP.restart();
        }
    }
    Serial.println(F(" connected!"));
    Serial.print(F("IP address set: "));
    Serial.println(WiFi.localIP()); //print LAN IP

    //init and get the time
    configTime(0, 0, ntpServer);
    setenv("TZ", timezoneEST, 1);
    time_t now;
    Serial.print("Obtaining NTP time: ");
    printLocalTime();
    while (now < 1510592825)
    {
        Serial.print(F("."));
        delay(500);
        time(&now);
    }
    Serial.print(F(" success!\nGot Time: "));
    printLocalTime(true);
    Serial.print(F("NTP time received!\n"));

    // Add constant tags - only once
    sensor.addTag("device", HOSTNAME);
    sensor.addTag("location", LOCATION);

    // Check Influxdb server connection
    if (client.validateConnection())
    {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(client.getServerUrl());
    }
    else
    {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(client.getLastErrorMessage());
    }
}

void loop()
{
    unsigned long currentMillis = millis();

    if (WiFi.status() == WL_CONNECTED)
    {                                                        //if we are connected to Eduroam network
        counter = 0;                                         //reset counter
        if (currentMillis - previousMillisWiFi >= 15 * 1000) // send data every 15 sec
        {
            printLocalTime(true);
            sendData();
            previousMillisWiFi = currentMillis;
            Serial.print(F("Wifi is still connected with IP: "));
            Serial.println(WiFi.localIP()); //inform user about his IP address
        }
    }
    else if (WiFi.status() != WL_CONNECTED)
    { //if we lost connection, retry
        WiFi.begin(ssid);

        Serial.printf("Connecting to WiFi: %s ", ssid);
        while (WiFi.status() != WL_CONNECTED)
        { //during lost connection, print dots
            delay(500);
            Serial.print(F("."));
            counter++;
            if (counter >= 60)
            { //30 seconds timeout - reset board
                ESP.restart();
            }
        }
        Serial.println(F(" connected!"));
    }
}