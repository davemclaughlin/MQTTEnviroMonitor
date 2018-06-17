#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>       // this is needed for display
#include <Adafruit_ILI9341.h>
#include <Wire.h>      // this is needed for FT6206
#include <Adafruit_FT6206.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define wifi_ssid "***"
#define wifi_password "***"

#define mqtt_server "192.168.1.146"
#define mqtt_user ""
#define mqtt_password ""

WiFiClient espClient;
PubSubClient client(espClient);
//
// Callback for MQTT
//
void callback(char* topic, byte* payload, unsigned int length);

long lastMsg = 0;

// The FT6206 uses hardware I2C (SCL/SDA)
Adafruit_FT6206 ctp = Adafruit_FT6206();

// The display also uses hardware SPI, plus #9 & #10
#define TFT_CS 15
#define TFT_DC 2
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

long msUpdateDisplay;

#define BLACK	0x000
#define BLUE	0x01F
#define RED		0xF800
#define GREEN	0x07E0
#define CYAN	0x07FF
#define ORANGE	0xFCE7
#define MAGENTA 0xF81F
#define YELLOW	0xFFE0
#define WHITE   0xFFFF
#define MAROON  0x801F
#define DRKRED	0x8000
#define BACK	0x001F

int airQuality;			// Value of PM2.5
float airTemp;
int airHum;
int airAqi = 1;
float airVOC;	

long msTimeout; 

void setup(void) {
	Serial.begin(115200);
	Serial.println(F("Cap Touch Paint!"));

	tft.begin();

	if (!ctp.begin(40)) {  // pass in 'sensitivity' coefficient
		Serial.println("Couldn't start FT6206 touchscreen controller");
	}

	Serial.println("Capacitive touchscreen started");

	tft.fillScreen(BACK);
	//
	// Main screen create
	//
	tft.setFont(NULL);
	tft.setTextColor(WHITE);
	tft.setTextSize(4);
	tft.setCursor(150, 28);
	tft.printf("PM");
	tft.setTextSize(2);
	tft.setCursor(200, 42);
	tft.printf("2.5");
	tft.setCursor(160, 60);
	tft.printf("ug/m3");
	tft.drawLine(0, 90, 239, 90, WHITE);
	tft.drawLine(0, 91, 239, 91, WHITE);
	// 
	// Colour box goes here
	//
	tft.drawLine(0, 120, 239, 120, WHITE);
	tft.drawLine(0, 121, 239, 121, WHITE);
	//
	// VOC goes here
	//
	tft.setTextSize(4);
	tft.setCursor(5, 148);
	tft.printf("VOC");
	tft.setTextSize(2);
	tft.setCursor(5, 183);
	tft.printf("mg/m3");

	tft.drawLine(0, 230, 239, 230, WHITE);
	tft.drawLine(0, 231, 239, 231, WHITE);

	tft.drawLine(125, 230, 125, 319, WHITE);
	tft.drawLine(126, 230, 126, 319, WHITE);
	//
	// Outside temp/hum goes here
	//
	tft.setTextSize(3);
	tft.setCursor(68, 238);
	tft.printf("C");
	tft.setTextSize(2);
	tft.setCursor(50, 234);
	tft.printf("o");

	tft.setTextSize(2);
	tft.setCursor(175, 234);
	tft.printf("o");
	tft.setCursor(190, 246);
	tft.printf("o");
	tft.drawLine(177, 256, 198, 238, WHITE);

	setup_wifi();
	client.setServer(mqtt_server, 1883);

	msUpdateDisplay = millis();		// Causes initial redraw
	
	msTimeout = millis() + 1000;	// Data timeout 10 seconds
}

void setup_wifi() {
	delay(10);
	// We start by connecting to a WiFi network
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(wifi_ssid);

	WiFi.begin(wifi_ssid, wifi_password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
}

void reconnect() {
	//
	// Loop until we're reconnected
	//
	while (!client.connected()) {
		Serial.print("Attempting MQTT connection...");
		//
		// Attempt to connect
		//
		if (client.connect("enviromondisplay")) {
			Serial.println("connected");

			client.subscribe("airout/quality");
			client.subscribe("airout/temp");
			client.subscribe("airout/hum");
			client.subscribe("airout/aqi");
			client.subscribe("airout/voc");

			client.setCallback(callback);
		}
		else {
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println(" try again in 5 seconds");
			//
			// Wait 5 seconds before retrying
			//
			delay(5000);
		}
	}
}

void loop() {
	char buf[20];

	if (!client.connected()) {
		reconnect();
	}
	client.loop();

	// Wait for a touch
	if (ctp.touched()) {
		// Retrieve a point  
		TS_Point p = ctp.getPoint();

		// flip it around to match the screen.
		p.x = map(p.x, 0, 240, 240, 0);
		p.y = map(p.y, 0, 320, 320, 0);

		// Print out the remapped (rotated) coordinates
		Serial.print("("); Serial.print(p.x);
		Serial.print(", "); Serial.print(p.y);
		Serial.println(")");
	}
	long now = millis();
	if (now - lastMsg > 5000) {
		lastMsg = now;
//		client.publish("test", "this works");
	}
	if (millis() >= msUpdateDisplay)
	{
		msUpdateDisplay = millis() + 1000;	// Every second

		if (millis() > msTimeout)
		{
			tft.setTextColor(RED, BACK);
		}
		else
		{
			tft.setTextColor(WHITE, BACK);
		}
		tft.setTextSize(8);
		tft.setCursor(5, 16);
		tft.printf("%3d", airQuality);
		//
		// VOC
		//
		tft.setTextSize(5);
		tft.setCursor(85, 155);
		tft.printf("%s", ftoa(airVOC, 1));
		//
		// Temp/HUm
		// 
		tft.setTextSize(5);
		tft.setCursor(5, 270);
		tft.printf("%s", ftoa(airTemp, 1));

		tft.setCursor(155, 270);
		tft.printf("%2d", (int) airHum);
		//
		// Draw a colour rectangle to indicate how bad it is
		//
		switch (airAqi)
		{
		case 1:
			tft.fillRect(5, 98, 230, 16, GREEN);
			break;
		case 2:
			tft.fillRect(5, 98, 230, 16, YELLOW);
			break;
		case 3:
			tft.fillRect(5, 98, 230, 16, ORANGE);
			break;
		case 4:
			tft.fillRect(5, 98, 230, 16, RED);
			break;
		case 5:
			tft.fillRect(5, 98, 230, 16, MAROON);
			break;
		case 6:
			tft.fillRect(5, 98, 230, 16, DRKRED);
			break;
		}
	}
}

int power(int base, int exp) {
	int result = 1;
	while (exp) { result *= base; exp--; }
	return result;
}

static char* ftoa(float num, uint8_t decimals) {
	// float to string; no float support in esp8266 sdk printf
	// warning: limited to 15 chars & non-reentrant
	// e.g., dont use more than once per os_printf call
	static char buf[16];
	int whole = num;
	int decimal = (num - whole) * power(10, decimals);
	if (decimal < 0) {
		// get rid of sign on decimal portion
		decimal -= 2 * decimal;
	}
	char pattern[10]; // setup printf pattern for decimal portion
	if(decimals == 1)
		os_sprintf(pattern, "%%d.%%1d");
	else
		os_sprintf(pattern, "%%d.%%0%dd", decimals);
	os_sprintf(buf, pattern, whole, decimal);
	return (char *)buf;
}

void callback(char* topic, byte* payload, unsigned int length) 
{
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");

	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println();

	if (length > 0)
	{
		payload[length] = 0;

		if (!strcmp(topic, "airout/quality"))		// Outside air quality
		{
			String msg = (char*)payload;

			airQuality = atoi((char*)payload);
		}
		if (!strcmp(topic, "airout/temp"))			// Outside temperature
		{
			String msg = (char*)payload;

			airTemp = atof((char*)payload);
		}
		if (!strcmp(topic, "airout/hum"))			// Outside humidity
		{
			String msg = (char*)payload;

			airHum = atof((char*)payload);
		}
		if (!strcmp(topic, "airout/aqi"))			// API indicator
		{
			String msg = (char*)payload;

			airAqi = atoi((char*)payload);
		}
		if (!strcmp(topic, "airout/voc"))			// VOC reading
		{
			String msg = (char*)payload;

			airVOC = atof((char*)payload);
		}
		msTimeout = millis() + 10000;
	}
}

