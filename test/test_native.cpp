#include <unity.h>
#include "topics.h"
#include <iostream>
#include <ArduinoJson.h>

#define XSTR(x) #x
#define STR(x) XSTR(x)

// Some simple tests

using namespace std;
int main(int argc, char **argv)
{
    UNITY_BEGIN();
    StaticJsonDocument<1000> doc;

    cout << "Testing parsing:" << endl;
    cout << MQTT_AUTO_DISC << endl;

    DeserializationError error = deserializeJson(doc, MQTT_AUTO_DISC);

    if (error) {
        cout << "deserializeJson() failed: " << endl;
        cout << error.c_str() << endl;
        TEST_FAIL_MESSAGE(error.c_str());
    }
    
    const char* name = doc["name"];
    cout << name << " " << STR(MQTT_CLIENT) << endl;
    TEST_ASSERT_EQUAL_STRING(name, STR(MQTT_CLIENT));

    const char* state_topic = doc["state_topic"];
    cout << state_topic << " " << STR(STATE_TOPIC) << endl;
    TEST_ASSERT_EQUAL_STRING(state_topic, STR(STATE_TOPIC));

    const char* avil_topic = doc["availability"]["topic"];
    cout << avil_topic << " " << STR(AVIL_TOPIC) << endl;
    TEST_ASSERT_EQUAL_STRING(avil_topic, STR(AVIL_TOPIC));

    return UNITY_END();
}