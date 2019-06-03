//-----------------------------------------------------------------------------------------------------
// This code is mainly from examples of WhiteBox Labs website with some modifications. For the original
// version, check on https://www.whiteboxes.ch/tentacle.
//-----------------------------------------------------------------------------------------------------

#include <Wire.h>                              // enable I2C.
#include <SoftwareSerial.h>
#define TOTAL_CIRCUITS 4                       // <-- CHANGE THIS | set how many I2C circuits are attached to the Tentacle

SoftwareSerial Zigbee(10, 11); // RX, TX

const unsigned int baud_host  = 9600;        // set baud rate for host serial monitor(pc/mac/other)
const unsigned int send_readings_every = 10000; // set at what intervals the readings are sent to the computer (NOTE: this is not the frequency of taking the readings!)
unsigned long next_serial_time;

char sensordata[30];                          // A 30 byte character array to hold incoming data from the sensors
byte sensor_bytes_received = 0;               // We need to know how many characters bytes have been received
byte code = 0;                                // used to hold the I2C response code.
byte in_char = 0;                             // used as a 1 byte buffer to store in bound bytes from the I2C Circuit.

int channel_ids[] = {100, 99, 97, 102};        // <-- CHANGE THIS. A list of I2C ids that you set your circuits to.
char *channel_names[] = {"EC", "pH", "DO", "RTD"};   // <-- CHANGE THIS. A list of channel names (must be the same order as in channel_ids[]) - only used to designate the readings in serial communications
String readings[TOTAL_CIRCUITS];               // an array of strings to hold the readings of each channel
int channel = 0;                              // INT pointer to hold the current position in the channel_ids/channel_names array

const unsigned int reading_delay = 1000;      // time to wait for the circuit to process a read command. datasheets say 1 second.
unsigned long next_reading_time;              // holds the time when the next reading should be ready from the circuit
boolean request_pending = false;              // wether or not we're waiting for a reading


void setup() {
  Serial.begin(baud_host);                  // Set the hardware serial port.
  pinMode(11, OUTPUT);
  Zigbee.begin(9600);
  Wire.begin();                          // enable I2C port.
  next_serial_time = millis() + send_readings_every;  // calculate the next point in time we should do serial communications
}



void loop() {
  do_sensor_readings();
  do_serial();

  // Do other stuff here (Blink Leds, update a display, etc)

}



// do serial communication in a "asynchronous" way, the string message is build as a dictionary type in Python
void do_serial() {
  if (millis() >= next_serial_time) {                // is it time for the next serial communication?
    String msg_send = "{'";
    msg_send = msg_send + channel_names[0] + "': \"" + readings[0] + "\", ";
    
    for (int i = 1; i < TOTAL_CIRCUITS - 1; i++) {    // loop through sensors 1 and 2
      msg_send = msg_send + "'" + channel_names[i] + "': " + readings[i] + ", ";
    }

    msg_send = msg_send + "'" + channel_names[TOTAL_CIRCUITS - 1] + "': " + readings[TOTAL_CIRCUITS - 1] + "}";
    
    Zigbee.println(msg_send);
    //Serial.println(msg_send);
    
    next_serial_time = millis() + send_readings_every;
  }
}


// take sensor readings in a "asynchronous" way
void do_sensor_readings() {
  if (request_pending) {                          // is a request pending?
    if (millis() >= next_reading_time) {          // is it time for the reading to be taken?
      receive_reading();                          // do the actual I2C communication
    }
  } else {                                        // no request is pending,
    channel = (channel + 1) % TOTAL_CIRCUITS;     // switch to the next channel (increase current channel by 1, and roll over if we're at the last channel using the % modulo operator)
    request_reading();                            // do the actual I2C communication
  }
}



// Request a reading from the current channel
void request_reading() {
  request_pending = true;
  Wire.beginTransmission(channel_ids[channel]); // call the circuit by its ID number.
  Wire.write('r');                        // request a reading by sending 'r'
  Wire.endTransmission();                      // end the I2C data transmission.
  next_reading_time = millis() + reading_delay; // calculate the next time to request a reading
}



// Receive data from the I2C bus
void receive_reading() {
  sensor_bytes_received = 0;                        // reset data counter
  memset(sensordata, 0, sizeof(sensordata));        // clear sensordata array;

  Wire.requestFrom(channel_ids[channel], 48, 1);    // call the circuit and request 48 bytes (this is more then we need).
  code = Wire.read();

  while (Wire.available()) {          // are there bytes to receive?
    in_char = Wire.read();            // receive a byte.

    if (in_char == 0) {               // if we see that we have been sent a null command.
      Wire.endTransmission();         // end the I2C data transmission.
      break;                          // exit the while loop, we're done here
    }
    else {
      sensordata[sensor_bytes_received] = in_char;  // load this byte into our array.
      sensor_bytes_received++;
    }
  }

  switch (code) {                          // switch case based on what the response code is.
    case 1:                               // decimal 1  means the command was successful.
      readings[channel] = sensordata;
      break;                                // exits the switch case.

    case 2:                                // decimal 2 means the command has failed.
      readings[channel] = "error: command failed";
      break;                                 // exits the switch case.

    case 254:                              // decimal 254  means the command has not yet been finished calculating.
      readings[channel] = "reading not ready";
      break;                                 // exits the switch case.

    case 255:                              // decimal 255 means there is no further data to send.
      readings[channel] = "error: no data";
      break;                                 // exits the switch case.
  }
  request_pending = false;                  // set pending to false, so we can continue to the next sensor
}
