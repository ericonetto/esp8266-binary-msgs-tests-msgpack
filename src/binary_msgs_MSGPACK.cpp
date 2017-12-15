#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <msgpck.h>


#define SSID ""
#define SSID_PASSWORD ""
#define SERVER_IP_ADDRESS ""

// MyData contains important data.
struct MyData {
  char name[10]{0};  // UTF-8 encoded
  bool planet = true;
  uint8_t number=0;
  double gravity=0;
} myData;



bool msgpack_read_double(Stream * s, double *d) {
  byte db;
  uint8_t read_size;
  bool b = true;
  if (s->readBytes(&db,1) != 1)
  return false;

  if (db == 0xcb) {
    read_size = 8;
    union double_to_byte {
      double d;
      byte b[8];
    } b2d;
    b = s->readBytes(&(b2d.b[7]),1) == 1;
    b = s->readBytes(&(b2d.b[6]),1) == 1;
    b = s->readBytes(&(b2d.b[5]),1) == 1;
    b = s->readBytes(&(b2d.b[4]),1) == 1;
    b = s->readBytes(&(b2d.b[3]),1) == 1;
    b = s->readBytes(&(b2d.b[2]),1) == 1;
    b = s->readBytes(&(b2d.b[1]),1) == 1;
    b = s->readBytes(&(b2d.b[0]),1) == 1;
    *d = b2d.d;
    return b;
  } else {
    return false;
  }
  return false;
}


void msgpck_write_double(Stream *s, double d) {
  union double_to_byte {
    double d;
    byte b[8];
  } d2b;
  d2b.d = d;
  s->write(0xcb);
  s->write(d2b.b[7]);
  s->write(d2b.b[6]);
  s->write(d2b.b[5]);
  s->write(d2b.b[4]);
  s->write(d2b.b[3]);
  s->write(d2b.b[2]);
  s->write(d2b.b[1]);
  s->write(d2b.b[0]);
}




//EXPECT {'name':'earth', 'planet':true, 'number': 3, 'gravity': 9.807};
bool loadDataClassic(MyData* myData, Stream* stream) {
  bool res = true;
  if(stream->available() > 0) {
    Serial.println("Stream avaiable");

    char buf[8];
    uint32_t map_size;
    uint32_t r_size;
    uint8_t number;
    double gravity;
    bool isPlanet=false;

    res &= msgpck_map_next(stream);
    if(!res)
    return false;
    res &= msgpck_read_map_size(stream, &map_size);
    if(!res)
    return false;
    res &= msgpck_read_string(stream, buf, 4, &r_size);
    if(!res)
    return false;

    if (strncmp(buf, "name",4) == 0) {
      if (msgpck_read_string(stream, buf, 5, &r_size)) {
        strncpy(myData->name, buf,r_size);
        myData->name[r_size] = '\0';
      } else {
        Serial.println("Text value was not text!" );
        return false;
      }
    }


    res &= msgpck_read_string(stream, buf, 6, &r_size);
    if(!res)
    return false;

    if (strncmp(buf, "planet", 6) == 0) {
      if (msgpck_read_bool(stream, &isPlanet)) {
        myData->planet=isPlanet;
      } else {
        Serial.println("planet was not a boolean!" );
        return false;
      }
    }


    res &= msgpck_read_string(stream, buf, 6, &r_size);
    if(!res)
    return false;


    if (strncmp(buf, "number",6) == 0) {
      if (msgpck_read_integer(stream, &number, 10)) {
        myData->number=number;
      } else {
        Serial.println("number was not a integer!" );
        return false;
      }
    }


    res &= msgpck_read_string(stream, buf, 7, &r_size);
    if(!res)
    return false;

    if (strncmp(buf, "gravity",7) == 0) {
      if (msgpack_read_double(stream, &gravity)) {
        myData->gravity=gravity;
      } else {
        Serial.println("gravity was not a float!" );
        return false;
      }
    }



  }
  return res;
}


void setup() {
  Serial.begin(115200);
  Serial.println("SETUP STARTED");                        //Serial connection
  WiFi.begin(SSID, SSID_PASSWORD);   //WiFi connection

  while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection completion

    delay(500);
    Serial.println("Waiting for connection");

  }
  Serial.println("SETUP FINISHED");
}

void loop() {

  if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
    HTTPClient http;
    //RECEIVE A messagepack MESSAGE

    Serial.print("[HTTP] begin RECEIVING msgpack..\n");
    // configure server and url
    http.begin("http://" + (String)SERVER_IP_ADDRESS + ":3000/messagepack");

    Serial.print("[HTTP] GET... ");

    // start connection and send HTTP header
    http.addHeader("Content-Type", "application/json");

    int httpCode =http.GET();
    if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if(httpCode == HTTP_CODE_OK) {

        // get tcp stream
        WiFiClient * stream = http.getStreamPtr();

        // get available data size
        size_t size = stream->available();

        if(size) {
          MyData dt;
          //loadMyData(&dt,stream);
          if(loadDataClassic(&dt, stream)){
            Serial.printf("Received> name: %s, planet: %s, number: %u, gravity: ", dt.name, dt.planet ? "true" : "false", dt.number);
            //Known issue: Arduino ide does not support %f
            Serial.println(dt.gravity,3);
          }

        }

      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();



    delay(5000);  //Send a request every 30 seconds



    //POST A MSGPACK MESSAGE

    Serial.print("[HTTP] begin posting a messagepack...\n");
    // configure server and url
    http.begin("http://" + (String)SERVER_IP_ADDRESS + ":3000/messagepack/unpack");

    Serial.print("[HTTP] POST...\n");
    char msgPackMessage[]={0x84, 0xa4, 0x6e, 0x61, 0x6d, 0x65, 0xa5, 0x65, 0x61,
      0x72, 0x74, 0x68, 0xa6, 0x70, 0x6c, 0x61, 0x6e, 0x65, 0x74, 0xc3, 0xa6,
      0x6e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x03, 0xa7, 0x67, 0x72, 0x61, 0x76,
      0x69, 0x74, 0x79, 0xcb, 0x40, 0x23, 0x9d, 0x2f, 0x1a, 0x9f, 0xbe, 0x77};

      // start connection and send HTTP header
      http.POST(msgPackMessage);
      if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] POST... code: %d\n", httpCode);

        // file found at server
        if(httpCode == HTTP_CODE_OK) {

          String payload = http.getString();
          Serial.println("Response:");
          Serial.println(payload);
          Serial.println("");

          Serial.print("[HTTP] connection closed or file end.\n");
        }
      } else {
        Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();


    }



    delay(30000);  //Send a request every 30 seconds

  }
