#ifndef INFO_H
#define INFO_H

// WiFi credentials
const char *SSID1 = "YOUR_SSID";
const char *PASS1 = "YOUR_PASS";

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

const int sizeOfEntry = 100;
const int oneSecond = 1000000;
const int framesPerSecond = 100;
const int frameDelay = 1000 / framesPerSecond;
const int secondsToSD = 10;

// https://lastminuteengineers.com/esp32-sleep-modes-power-consumption/

const char *header =
	"Timestamp;Index;AccelX;AccelY;AccelZ;gyroX;GyroY;GyroZ;Temperature";

char *payload_combined;
char *payload_entry;
char delimiter = '=';

const int asyncport = 80;
const char *PARAM_INPUT_1 = "student";
const char *PARAM_INPUT_2 = "style";
const char *FILE_INPUT = "file";

long long previous_time;
long long current_time;

char *return_sample;
String studentid;
String style;
String folder_path;
String file_download;

volatile int programState = 0;
volatile int frameOnTimer = 0;
volatile int frameDelayIndex = 0;

int leftForWritting = 0;

long indexOfFrame = 0;

File file;

hw_timer_t *timer_frame = NULL;

#define SERIAL_BEGIN Serial.begin
#define SERIAL_PRINTLN Serial.println

#endif
