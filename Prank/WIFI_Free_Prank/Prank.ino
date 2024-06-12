#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <SPIFFS.h> 

DNSServer dnsServer;
AsyncWebServer server(80);

// Change these to your router's SSID and Password
const char* ssid = "SSID OF WIFI";
const char* password = "PASSWORD";

const char ap_ssid[] = "Free_WIFI";
const char ap_password[] = "";

void formatSPIFFS() {
    Serial.println("Formatting SPIFFS...");
    if (SPIFFS.format()) {
        Serial.println("SPIFFS formatted successfully.");
    } else {
        Serial.println("Failed to format SPIFFS.");
    }
}

bool mountSPIFFS() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS. Formatting...");
        formatSPIFFS();
        if (!SPIFFS.begin()) {
            Serial.println("An Error has occurred while mounting SPIFFS after formatting");
            return false;
        }
    }
    Serial.println("SPIFFS mounted successfully.");
    return true;
}

void checkFileExists() {
    if (SPIFFS.exists("/index.html")) {
        Serial.println("index.html exists on SPIFFS.");
    } else {
        Serial.println("index.html does NOT exist on SPIFFS.");
    }
}

class CaptiveRequestHandler : public AsyncWebHandler {
public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request) {
        return true;
    }

    void handleRequest(AsyncWebServerRequest *request) {
        Serial.println("Serving captive portal page");
        if (SPIFFS.exists("/index.html")) {
            request->send(SPIFFS, "/index.html", String(), false);
        } else {
            request->send(200, "text/html", "<html><body><h1>File Not Found</h1></body></html>");
        }
    }
};

void setupServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("Client requested /");
        if (SPIFFS.exists("/index.html")) {
            request->send(SPIFFS, "/index.html", String(), false);
        } else {
            request->send(200, "text/html", "<html><body><h1>File Not Found</h1></body></html>");
        }
    });

    server.onNotFound([](AsyncWebServerRequest *request){
        Serial.println("Client requested not found page, redirecting to main page");
        if (SPIFFS.exists("/index.html")) {
            request->send(SPIFFS, "/index.html", String(), false);
        } else {
            request->send(200, "text/html", "<html><body><h1>File Not Found</h1></body></html>");
        }
    });
}

void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Serial.println("Client Connected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.println("Client Disconnected");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.println("Client IP Assigned");
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    // Mount SPIFFS
    if (mountSPIFFS()) {
        checkFileExists();
    } else {
        Serial.println("Failed to mount and format SPIFFS.");
    }

    // Connect to the router
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to the WiFi network");

    // Set up AP mode
    Serial.println("Setting up AP Mode");
    WiFi.softAP(ap_ssid, ap_password);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    // Set up event handler for client connections
    WiFi.onEvent(WiFiEvent);

    Serial.println("Setting up Async WebServer");
    setupServer();

    Serial.println("Starting DNS Server");
    dnsServer.start(53, "*", WiFi.softAPIP());

    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER); //only when requested from AP
    server.begin();

    Serial.println("All Done!");
}

void loop() {
    dnsServer.processNextRequest();
}
