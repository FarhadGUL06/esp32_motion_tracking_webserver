#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <SD.h>
#include <SPI.h>
#include <Wifi.h>
#include <esp_wifi.h>
#include <time.h>

#include "BMI085.h"
#include "info.h"

// For NEOPIXEL
Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// FOR BMI085
BMI085Gyro gyro(Wire, 0x68);
BMI085Accel accel(Wire, 0x18);

// For SD CARD

// For Async Web Server
AsyncWebServer server(asyncport);

void light_off()
{
	pixel.setBrightness(0);
	pixel.fill(0x000000);
	pixel.show();
}

void light_green_on()
{
	pixel.setBrightness(5);
	pixel.fill(0x4CBB17);
	pixel.show();
}

void light_red_on()
{
	pixel.setBrightness(5);
	pixel.fill(0xFF0000);
	pixel.show();
}

int connectToWifi(const char *ssid, const char *password)
{
	SERIAL_PRINTLN("Connecting to WIFI");
	WiFi.begin(ssid, password);
	esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
	while (WiFi.status() != WL_CONNECTED) {
		delay(10);
	}
	if (WiFi.status() == WL_CONNECTED) {
		SERIAL_PRINTLN("Connected to WIFI");
		SERIAL_PRINTLN(WiFi.localIP());
		return 1;
	} else {
		return 0;
	}
}

long long getCurrentTime()
{
	// Calculate current time
	struct tm timeinfo;
	long long timestamp = 0;
	if (!getLocalTime(&timeinfo)) {
		SERIAL_PRINTLN("Failed to obtain time");
		return timestamp;
	}

	// Transform in seconds
	timestamp = mktime(&timeinfo);
	return timestamp;
}

void getEntryInfo(long index)
{
	// Get BMI085 data and timestamp
	accel.readSensor();
	gyro.readSensor();
	current_time = getCurrentTime();

	float accelX = accel.getAccelX_mss();
	float accelY = accel.getAccelY_mss();
	float accelZ = accel.getAccelZ_mss();

	float gyroX = gyro.getGyroX_rads();
	float gyroY = gyro.getGyroY_rads();
	float gyroZ = gyro.getGyroZ_rads();
	float temp = accel.getTemperature_C();

	memset(payload_entry, 0, sizeOfEntry);
	sprintf(payload_entry, "%lli;%li;%f;%f;%f;%f;%f;%f;%d\n", current_time,
			index, accelX, accelY, accelZ, gyroX, gyroY, gyroZ, (int)temp);
}

void IRAM_ATTR onTimer()
{
	if (frameOnTimer == 1) {
		if (frameDelayIndex == 1) {
			frameDelayIndex = 0;
		} else {
			frameDelayIndex = 1;
		}
	}
}

void getMultipleFrames()
{
	previous_time = getCurrentTime();
	while (getCurrentTime() == previous_time) {
		// Wait for time to change
	}
	frameOnTimer = 1;
	int hasToWrite = 0;
	while (programState == 1) {
		for (int i = 0; i < secondsToSD * framesPerSecond; i++) {
			frameDelayIndex = 0;
			timerWrite(timer_frame, 0);
			if (hasToWrite == 1) {
				file.print(payload_combined);
				memset(payload_combined, 0,
					   sizeOfEntry * secondsToSD * framesPerSecond);
				hasToWrite = 0;
				leftForWritting = 0;
			}
			getEntryInfo(indexOfFrame);
			indexOfFrame++;
			strcat(payload_combined, payload_entry);
			leftForWritting = 1;

			if (i == secondsToSD * framesPerSecond - 1) {
				hasToWrite = 1;
				break;
			}
			if (programState == 0) {
				break;
			}
			while (frameDelayIndex == 0) {
				// Wait for timer to trigger
			}
		}
	}
	if (leftForWritting == 1) {
		file.print(payload_combined);
		memset(payload_combined, 0,
			   sizeOfEntry * secondsToSD * framesPerSecond);
		hasToWrite = 0;
		leftForWritting = 0;
	}

	SERIAL_PRINTLN("Program stopped!");
	frameOnTimer = 0;
	timerAlarmDisable(timer_frame);
	file.close();
}

void start_thread()
{
	// Create one thread for writing to file
	pthread_t th[1];
	int thread_id[1];
	thread_id[0] = 0;
	int iret;
	memset(payload_combined, 0, sizeOfEntry * secondsToSD * framesPerSecond);
	indexOfFrame = 0;
	timerAlarmEnable(timer_frame);
	getMultipleFrames();
}

void configureTimerFrame()
{
	timer_frame = timerBegin(0, 80, true);
	timerAttachInterrupt(timer_frame, &onTimer, true);
	timerAlarmWrite(timer_frame, oneSecond / framesPerSecond, true);
}

void initialise_stuff()
{
	SERIAL_BEGIN(9600);
	SERIAL_PRINTLN("Turning on!");
	pixel.begin();

	// When loading => red
	light_red_on();

	while (accel.begin() != 1 || gyro.begin() != 1) {
		SERIAL_PRINTLN("Initialising...");
		delay(100);
	}

	int statussd = SD.begin(4);
	while (!statussd) {
		SERIAL_PRINTLN("Initialising SD Card: status = ");
		SERIAL_PRINTLN(statussd);
		delay(10);
		statussd = SD.begin(4);
	}

	// Connect to Wifi
	int status = 0;
	while (!status) {
		status = connectToWifi(SSID1, PASS1);
	}

	delay(100);

	// Configure NTP Client
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	configureTimerFrame();
	current_time = getCurrentTime();
	// Initialise memory
	payload_combined = (char *)malloc(sizeOfEntry * secondsToSD *
									  framesPerSecond * sizeof(char));

	payload_entry = (char *)malloc(sizeOfEntry * sizeof(char));
	// When ready => green
	light_green_on();

	delay(100);

	// Turn the light off
	light_off();
	SERIAL_PRINTLN("Initialised!");
}

void start_trial()
{
	folder_path = "/" + studentid + delimiter + style + delimiter +
				  getCurrentTime() + ".txt";
	file = SD.open(folder_path.c_str(), FILE_APPEND);

	if (file) {
		file.println(header);
	} else {
		// if the file didn't open, print an error:
		SERIAL_PRINTLN("error opening file");
	}
}

void setup()
{
	initialise_stuff();

	// <ESP_IP>/start?student=<inputMessage1>&style=<inputMessage2>
	server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request) {
		SERIAL_PRINTLN("Program started!");
		if (programState == 1) {
			request->send(200, "text/plain", "Program already started!");
			return;
		}
		if (request->hasParam(PARAM_INPUT_1) &&
			request->hasParam(PARAM_INPUT_2)) {
			studentid = request->getParam(PARAM_INPUT_1)->value();
			style = request->getParam(PARAM_INPUT_2)->value();
		}

		request->send(200, "text/plain", "Program started!");
		start_trial();
		programState = 1;
	});
	// <ESP_IP>/isalive
	server.on("/isalive", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(200, "text/plain", "Yes");
	});

	// <ESP_IP>/stop
	server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
		if (programState == 0) {
			request->send(200, "text/plain", "Program already stopped!");
			return;
		}
		programState = 0;
		request->send(200, "text/plain", file.name());
	});

	// <ESP_IP>/download?file=<inputMessage1>
	server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
		SERIAL_PRINTLN("Program started!");
		if (request->hasParam(FILE_INPUT)) {
			file_download = request->getParam(FILE_INPUT)->value();
		}
		file_download = "/" + file_download;
		File to_send = SD.open(file_download.c_str(), FILE_READ);
		if (!to_send) {
			SERIAL_PRINTLN("File doesn't exist!");
			request->send(200, "text/plain", "File doesn't exist!");
			return;
		}

		SERIAL_PRINTLN("File exists!");
		AsyncWebServerResponse *response = request->beginChunkedResponse(
			"text/plain",
			[to_send](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
				maxLen = 512;
				File SDLambdaFile = to_send;
				return SDLambdaFile.read(buffer, maxLen);
			});
		request->send(response);
	});

	server.begin();
	Serial.println("[APP] Free memory: " + String(esp_get_free_heap_size()) +
				   " bytes");
}

void loop()
{
	if (programState == 1) {
		start_thread();
	}
}
