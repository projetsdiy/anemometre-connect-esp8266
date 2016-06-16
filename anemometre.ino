
/*
 * Anémomètre et girouette connecté à base d'ESP8266 (Wemos d1 mini)
 *  
 * Librairies nécessaires :
 *  - ESP8266Wifi : https://github.com/ekstrand/ESP8266wifi 
 *  - PubSubClient
 * 
 * http://www.projetsdiy.fr - Juin 2016 - Version 1.0
 * Licence : MIT
 * 
  */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
/*
 * Variables et paramètres de connexion WiFi
 * 
*/
#define wifi_ssid "SSID"
#define wifi_password "MOT_DE_PASSE"
#define mqtt_server "IP_MQTT"
#define mqtt_user "guest"     //s'il a été configuré sur Mosquitto
#define mqtt_password "guest" //idem

long lastMsg = 0;     //Horodatage du dernier message publié sur MQTT
long lastRecu = 0;

#define vitessems_topic "anemometre/vitesseVentMS"        // Topic Vitesse du vent (en m/s)
#define vitessekmh_topic "anemometre/vitesseVentKMH"      // Topic Vitesse du vent (en km/h)
#define direction_topic "anemometre/directionVent"    // Topic Direction du vent (lettre de la direction)

//Création des objets
   
WiFiClient espClient;
PubSubClient client(espClient);

volatile int rpmcount = 0;
volatile float vitVentMS = 0;
volatile float vitVentKMH = 0;
int rpm = 0;
unsigned long lastmillis = 0;
bool debug = false;

int pinBit0 = D7;
int pinBit1 = D6;
int pinBit2 = D5;
int pinBit3 = D4;
int pinBit4 = D3;
int pinHall = D2;

int bit0 = 0;
int bit1 = 0;
int bit2 = 0;
int bit3 = 0;
int bit4 = 0;

int dureeMesVitVent = 5000; // en ms, durée de mesure de la vitesse du vent. Choisir un multiple de 1000 pour le calcul de la vitesse du vent

//unsigned char girouette = 0;
byte girouette = 0;
String dirVent = "N";

void setup() {
  
  // put your setup code here, to run once:
  Serial.begin(9600);
  attachInterrupt(pinHall, rpm_vent, FALLING);
  pinMode(pinBit0, INPUT);
  pinMode(pinBit1, INPUT);
  pinMode(pinBit2, INPUT);
  pinMode(pinBit3, INPUT);
  pinMode(pinBit4, INPUT);
  //attachInterrupt(D7, bit0, FALLING);  

  setup_wifi();           //On se connecte au réseau wifi
  client.setServer(mqtt_server, 1883);    //Configuration de la connexion au serveur MQTT
  client.setCallback(callback);  //La fonction de callback qui est executée à chaque réception de message   
  
  Serial.print("GO");
}

//Connexion au réseau WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connexion a ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connexion WiFi etablie ");
  Serial.print("=> Addresse IP : ");
  Serial.print(WiFi.localIP());
}

//Reconnexion
void reconnect() {
  //Boucle jusqu'à obtenur une reconnexion
  while (!client.connected()) {
    Serial.print("Connexion au serveur MQTT...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("OK");
    } else {
      Serial.print("KO, erreur : ");
      Serial.print(client.state());
      Serial.println(" On attend 5 secondes avant de recommencer");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  getVitesseVent();
   
  getDirVent();
    
  client.publish(vitessems_topic, String(vitVentMS).c_str(), true);   // Publie la vitesse du vent en m/s
  client.publish(vitessekmh_topic, String(vitVentKMH).c_str(), true); // La vitesse du vent en km/h
  client.publish(direction_topic, String(dirVent).c_str(), true);     // La direction du vent
 
  //ESP.deepSleep(1000000 * 10, WAKE_NO_RFCAL); //Pour la prochaine version futur 
  //delay(500); //attente activation du deep sleep
  delay(2000);
}


void getVitesseVent() {
  if (millis() - lastmillis >= dureeMesVitVent ){ 
    detachInterrupt(pinHall); 

    rpm = rpmcount * ( 60 / ( dureeMesVitVent / 1000 ) ); 
    
    if ( rpm > 0 ) {
      vitVentKMH = ( rpm + 6.174 ) / 8.367;
      vitVentMS = ( ( ( rpm + 6.174 ) / 8.367 ) * 1000 ) / 3600; 
    } else {
      vitVentKMH = 0;
      vitVentMS = 0;
    }
    Serial.println(vitVentKMH);

    rpmcount = 0;           // Redémarre le compte tour
    lastmillis = millis();  // et réinitialise le chrono
    attachInterrupt(pinHall, rpm_vent, FALLING); // Rélance l'interruption du compte tour

  }


}

void getDirVent(){
  bit0 = not(digitalRead(pinBit0));
  bit1 = not(digitalRead(pinBit1));
  bit2 = not(digitalRead(pinBit2));
  bit3 = not(digitalRead(pinBit3));
  bit4 = not(digitalRead(pinBit4));
  
  girouette =  (bit4 * 16) + (bit3 * 8) + (bit2 * 4) + (bit1 * 2) + bit0;

  Serial.print(bit0); Serial.print(bit1); Serial.print(bit2); Serial.print(bit3); Serial.print(bit4);
  Serial.print(" => ");
  Serial.print(girouette);
  Serial.print(" Orientation");
  switch (girouette) {
    case 24: dirVent="N"; break;
    case 10: dirVent="NE"; break;
    case 18: dirVent="E"; break;
    //case 18: dirVent="SE"; break;
    case 16: dirVent="S"; break;
    case 2: dirVent="SW"; break;
    case 26: dirVent="O"; break;
    case 8: dirVent="NO"; break;
    default: dirVent="N"; break; 
  }
  Serial.println(dirVent);
}

void rpm_vent(){ 
  //Serial.println("Tour"),
  rpmcount++;
}


void callback(char* topic, byte* payload, unsigned int length) {

}
