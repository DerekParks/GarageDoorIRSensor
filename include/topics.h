
#define AVIL_TOPIC1 home/garageNorth/avail
#define STATE_TOPIC1 home/garageNorth/state
#define ALERT_TOPIC1 home/garageNorth/alert

#define AVIL_TOPIC2 home/garageSouth/avail
#define STATE_TOPIC2 home/garageSouth/state
#define ALERT_TOPIC2 home/garageSouth/alert

#define MQTT_CLIENT garage_door_esp

// Auto discover is not well documented. I wasted a lot of time on it. Giving up for now.
//#define CONFIG_TOPIC homeassistant/cover/garageNorth/config
//constexpr char const* MQTT_AUTO_DISC = "{\"name\":\"Garage Door North\", \"unique_id\": \"garageNorth\", \"state_topic\":\"homeassistant/cover/garageNorth/state\",\"availability\":{\"topic\":\"homeassistant/cover/garageNorth/availability\"},\"payload_available\":\"Online\",\"payload_not_available\":\"Offline\",\"state_opening\":\"opening\",\"state_closing\":\"closing\",\"state_open\":\"open\",\"state_closed\":\"closed\"}";