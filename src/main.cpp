//=====================================================================
// LVGL : How-to
//      : M5Core2 slow fps. Scrol slider breaks when moving horizontally
// 2 Dec,2020
// https://forum.lvgl.io
//  /t/m5core2-slow-fps-scrol-slider-breaks-when-moving-horizontally/3931
// Arduino IDE 1.8.15
// https://github.com/mhaberler/m5core2-lvgl-demo
// Check : 2021.06.13 : macsbug
// https://macsbug.wordpress.com/2021/06/18/how-to-run-lvgl-on-m5stack-esp32/
//=====================================================================

#include <Arduino.h>
#include "view.h"
#include "networking.h"
#include "sideled.h"
#include <Wire.h>
#include <SPI.h>
#include <VL53L0X.h>
#include <iostream>
#include <chrono>

lv_obj_t *labelValue;

VL53L0X sensor;

void init_gui_elements()
{
  labelValue = add_label(" ", 20, 20);
}

void init_sensor()
{
  Wire.begin();
  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1)
    {
    }
  }
  sensor.startContinuous();
}

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Testing\n");

  // Parse Payload into Colour string
  char *buf = (char *)malloc((sizeof(char) * (length + 1)));
  memcpy(buf, payload, length);
  buf[length] = '\0';
  String payloadS = String(buf);
  payloadS.trim();
  Serial.printf("%s = %s\n", String(topic), payloadS);

  if (String(topic) == "parking_garage")
  {
    if (payloadS == "Free")
    {
      lv_label_set_text(labelValue, "2 Free Parkingspaces");
    }
    else if (payloadS == "Used")
    {
      lv_label_set_text(labelValue, "0 Free Parkingspaces");
    }
  }
}

void setup()
{
  init_m5();
  init_display();
  Serial.begin(115200);
  // Uncomment the following lines to enable WiFi and MQTT
  lv_obj_t *wifiConnectingBox = show_message_box_no_buttons("Connecting to WiFi...");
  lv_task_handler();
  delay(5);
  setup_wifi();
  mqtt_init(mqtt_callback);
  close_message_box(wifiConnectingBox);
  init_gui_elements();
  init_sideled();
  init_sensor();

  set_sideled_state(0, 10, SIDELED_STATE_ON);
}

long nextSensorRead = 0;
unsigned long next_lv_task = 0;
//=====================================================================
void loop()
{
  if (next_lv_task < millis())
  {
    lv_task_handler();
    next_lv_task = millis() + 5;
  }
  if (nextSensorRead < millis())
  {
    unsigned long millimeter_distance = (unsigned long)sensor.readRangeContinuousMillimeters();

    if (sensor.timeoutOccurred())
    {
      Serial.print(" TIMEOUT");
    }

    if (millimeter_distance >= 170)
    {
      mqtt_publish("parking_garage", "Free");
      set_sideled_color(0, 10, CRGB::Green);
    }
    else
    {
      mqtt_publish("parking_garage", "Used");
      set_sideled_color(0, 10, CRGB::Red);
    }

    Serial.println();
    nextSensorRead = millis() + 2000;
    mqtt_loop();
  }
}
//=====================================================================
