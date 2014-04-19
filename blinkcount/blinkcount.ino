
/*
 * Arduino code to count blinks on fototransistors connected
 * to digital IO pins.
 *
 * Osram SFH 300 Silicon NPN Phototransistor appears to work fine.
 * Connect the short lead (collector) to the IO pin and
 * the long lead (emitter) to a ground pin of the Arduino.
 *
 * This little program counts how many blinks the transistor
 * has detected, and reports the counters over USB serial every
 * 5 seconds.
 *
 * The green led on the Arduino also blinks every time a blink
 * is detected on any of the sensors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 * Foundation; either version 1, or (at your option) any later version, or
 * b) the "Artistic License".
 *
 */
#include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
#include <Xively.h>

// LED pin
#define LED   13

// How many sensors?
#define SENSORS 1

// Arduino pin numbers for each sensor,
// port 0 ... port 4, for 5 sensors:
int pins[SENSORS] = { 12 };

// the current state of each sensor (light or no light seen?)
int state[SENSORS];

// how many blinks have been counted on each sensor
int blinks[SENSORS];

// Reporting interval on serial port, in milliseconds, every minute
#define REPORT_INTERVAL 60000

// assign a MAC address for the ethernet controller.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

char xivelyKey[] = "XaoqDBEaH6OGE4peiK1nSsCKbfXooAHA1Vwx6JoDRFt80YUk";

#define FEED_ID 198193412

// Power in watts, blinks per minute
char myIntStream[] = "power";

char server[] = "api.xively.com";

XivelyDatastream datastreams[] = {
  // Int datastreams are set up like this:
  XivelyDatastream(myIntStream, strlen(myIntStream), DATASTREAM_INT)
};

XivelyFeed feed(FEED_ID, datastreams, 1);

EthernetClient client;

XivelyClient xivelyclient(client);
    
void setup()
{
  Serial.begin(9600);
  Serial.println("Hello world");
    /* initialize each sensor */ 
    for (int i = 0; i < SENSORS; i++) {
        /* Set the pin to input, and enable internal pull-up by
         * writing a 1. This way we won't need any other components.
         */
        pinMode(pins[i], INPUT);
        digitalWrite(pins[i], 1); // pull up
        state[i] = HIGH;
        blinks[i] = 0;
    }
    Serial.println("Waiting for IP");
    // start the Ethernet connection:
    while (Ethernet.begin(mac) == 0) {
      Serial.println("Failed to configure Ethernet using DHCP, waiting");
      delay(15000);
    }
    Serial.println("IP obtained");
}

void loop()
{
  Serial.println("In main loop, counting blinks");
    int i;
    int t;
    int ledloops = 0;
    int led = LOW;
    unsigned long loops = 0;
    unsigned long next_print = millis() + REPORT_INTERVAL;
    
    // busy loop eternally
    while (1) {
        // go through each sensor
        for (i = 0; i < SENSORS; i++) {
            
            // read current value
            t = digitalRead(pins[i]);
            
            // check if state changed:
            if (t != state[i]) {
                state[i] = t;
                // no light to light shows as HIGH to LOW change on the pin,
                // as the photodiode starts to conduct
                if (t == LOW) {
                    blinks[i] += 1;
                    
                    // trigger arduino's own led up to indicate we have detected
                    // a blink
                    led = HIGH;
                    ledloops = 0;
                }
            }
        }
        
        // control my own led - keep it up for 4000 rounds (short blink)
        if (led == HIGH) {
             if (ledloops == 0)
                 digitalWrite(LED, HIGH);
             ledloops++;
             if (ledloops == 4000) {
                 digitalWrite(LED, LOW);
                 led = LOW;
             }
        }
        
        // Every 10000 rounds (quite often), check current uptime
        // and if REPORT_INTERVAL has passed, print out current values.
        // If this would take too long time (slow serial speed) or happen
        // very often, we could miss some blinks while doing this.
        loops++;
        if (loops == 10000) {
            loops = 0;
            
            if (millis() >= next_print) {
                next_print += REPORT_INTERVAL;
                Serial.println("+"); // start marker
                for (int i = 0; i < SENSORS; i++) {
                    Serial.print(i, DEC);
                    Serial.print(" ");
                    Serial.println(blinks[i], DEC);
                    blinks[i] = 0;
                }
                Serial.println("-"); // end marker
                datastreams[0].setInt(blinks[0]);
                Serial.println("Uploading it to Xively");
                int ret = xivelyclient.put(feed, xivelyKey);
                Serial.print("xivelyclient.put returned ");
                Serial.println(ret);                
            }
        }
    }
}
