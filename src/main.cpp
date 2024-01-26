// Copyright 2024 Craig Petchell

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino.h>
#include <WiFi.h>
#include <ETH.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <string.h>
#include <IRsend.h>

#include <blaster_config.h>
#include <ir_message.h>
#include <ir_queue.h>
#include <ir_task.h>
#include <secrets.h>
#include <web_task.h>
#include <webserver_task.h>
#include <global_config.h>
#include <config_file.h>

QueueHandle_t irQueueHandle;

TaskHandle_t webTaskxHandle = NULL;
TaskHandle_t webServerTaskxHandle = NULL;

global_context_t globalContext;

void setupWebTask(String macAddress)
{
    if (webTaskxHandle != NULL)
    {
         vTaskDelete(webTaskxHandle);
         webTaskxHandle = NULL;
    }
    if (webServerTaskxHandle != NULL)
    {
         vTaskDelete(webServerTaskxHandle);
         webServerTaskxHandle = NULL;
    }
    Serial.printf("MAC address: %s\nLaunching websocket server and web server...\n", macAddress.c_str());
    // Create a task to handle web requests from remote RC2
    const BaseType_t  webTaskHandle = xTaskCreatePinnedToCore(
        TaskWeb, "Task Web/Websocket server",
        32768, (void*)macAddress.c_str(), 2, &webTaskxHandle, 0
    );
    const BaseType_t  webServerTaskHandle = xTaskCreatePinnedToCore(
        TaskWebServer, "Task Web server",
        32768, &globalContext, 3, &webServerTaskxHandle, 0
    );
}


// Method called to initialize Wifi connection
void setupWifi()
{
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    const char *custom_ssid = globalContext.config[CONFIG_FILE_WIFI_SSID_ENTRY];
    const char *custom_password = globalContext.config[CONFIG_FILE_WIFI_PASSWORD_ENTRY];
    if (strlen(custom_ssid) > 0 && strlen(custom_password) > 0)
    {
      Serial.printf("Using custom SSID %s and password %s to connect to Wifi\n", custom_ssid, custom_password);
      WiFi.begin(custom_ssid, custom_password);
    }
    else
    {
      WiFi.begin(wifi_ssid, wifi_password);
    }
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.printf("WiFi Failed!\n");
        return;
    }
    globalContext.wifi_connected = true;
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString());
    // (Re)Initialize the webtask to the new adapter
    setupWebTask(WiFi.macAddress().c_str());
}

void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname(HOSTNAME);
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      globalContext.eth_connected = true;
      // Disconnect from Wifi once Ethernet IP is obtained
      WiFi.disconnect(true);
      globalContext.wifi_connected = false;
      // (Re)Launch webserver task
      setupWebTask(ETH.macAddress().c_str());
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      globalContext.eth_connected = false;
      // Connect to Wifi when Ethernet is disconnected
      setupWifi();
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      globalContext.eth_connected = false;
      break;
    default:
      break;
  }
}

// Main setup method
void setup()
{
    Serial.begin(115200);

    globalContext.eth_connected = false;
    globalContext.wifi_connected = false;
    // Load config from config.json inside flash memory if any otherwise default empty config
    globalContext.config = loadConfig();

    // ETH
    WiFi.onEvent(WiFiEvent);
    // Enable Wifi in case Ethernet is not connected at startup, it will be disconnected once the Ethernet IP is obtained
    setupWifi();
    /* Olimex ESP32-POE
      ethernet:
      type: LAN8720
      mdc_pin: GPIO23
      mdio_pin: GPIO18
      clk_mode: GPIO17_OUT
      phy_addr: 0
      power_pin: GPIO12
    */
    // BUT ETH.begin() does not work properly with Olimex boards => had to change the clk_mode from GPIO17_OUT to ETH_CLOCK_GPIO0_OUT
    // TODO : make it configurable in a header
    ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLOCK_GPIO0_OUT);    

    // Create the queue which will have <QueueElementSize> number of elements, each of size `message_t` and pass the address to <QueueHandle>.
    // TODO : Damien question to developer => why the queue size is limited to 2 ? the task could handle multiple IR codes in the queue sequentially ?
    irQueueHandle = xQueueCreate(IR_QUEUE_SIZE, sizeof(ir_message_t));

    // Check if the queue was successfully created
    if (irQueueHandle == NULL)
    {
        Serial.println("Queue could not be created. Halt.");
        while (1)
            delay(1000); // Halt at this point as is not possible to continue
    }
    
    // Create a task to handle IR commands to send
    const BaseType_t irTaskHandle = xTaskCreatePinnedToCore(
        TaskSendIR, "Task IR send task",
        32768, NULL, 3 /* highest priority */, NULL, 1
    );

}

void loop()
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}