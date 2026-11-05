#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "secrets.h"

const int led_pin = 2;

const char* TOPIC_CMD    = "casa/quarto/ventilador/comando";
const char* TOPIC_STATE  = "casa/quarto/ventilador/estado";
const char* TOPIC_STATUS = "casa/quarto/ventilador/status";

struct Comando {
    char name[20];
    uint32_t comando;
};


struct Comando comandos[] = { { "power", 0xFFA25D },
                              { "luz", 0xFF629D },
                              { "luz_fraca", 0xFFE21D },
                              { "luz_laranja", 0xFF22DD },
                              { "luz_amarela", 0xFF02FD },
                              { "luz_branca", 0xFFC23D },
                              { "menos_claridade", 0xFFE01F },
                              { "mais_claridade", 0xFF906F },
                              { "grd_amarela", 0xFF6897 },
                              { "grd_branca", 0xFFB04F },
                              { "ventilador", 0xFF18E7 },
                              { "velocidade_1", 0xFF10EF },
                              { "velocidade_2", 0xFF38C7 },
                              { "velocidade_3", 0xFF5AA5 },
                              { "timer_1h", 0xFF42BD },
                              { "timer_2h", 0xFF4AB5 },
                              { "timer_4h", 0xFF52AD } };
int numComandos = sizeof(comandos) / sizeof(comandos[0]);

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long ultimaTentativa = 0;
const uint16_t kIrLed = 18;

bool sistemaLigado = false;
bool luzAcesa = false;
int velocidade = 0;

IRsend irsend(kIrLed);

void regras(const char* nomeComando){
  if (strcmp(nomeComando, "power") == 0){
    sistemaLigado = !sistemaLigado;
    luzAcesa = sistemaLigado;
    velocidade = 0;
    return;
  }

  if(!sistemaLigado) return;

  if (strcmp(nomeComando, "luz") == 0){
    luzAcesa = !luzAcesa;
    return;
  }

  if (strcmp(nomeComando, "ventilador") == 0){
    if (velocidade == 0) {
      velocidade = 1;
    } else {
      velocidade = 0;
    }
    return;
  }

  if (strcmp(nomeComando, "velocidade_1") == 0){
    velocidade = 1;
    return;
  }

  if (strcmp(nomeComando, "velocidade_2") == 0){
    velocidade = 2;
    return;
  }

  if (strcmp(nomeComando, "velocidade_3") == 0){
    velocidade = 3;
    return;
  }
}

void imprimirEstado(const char* cmd) {
  Serial.printf("%-14s sistema=%d luz=%d velocidade=%d\n",
                cmd, sistemaLigado, luzAcesa, velocidade);
}

void publicarEstado(){
  char msg[96];
  snprintf(msg, sizeof(msg),
           "{\"sistema\":%s,\"luz\":%s,\"velocidade\":%d}",
           sistemaLigado ? "true" : "false",
           luzAcesa ? "true" : "false",
           velocidade);
  client.publish(TOPIC_STATE, msg, true);
}

void callback(char* topic, byte* payload, unsigned int length){
  char mensagem[64]; // buffer local de ate 63 caracteres uteis mais o \0

  if (length>= sizeof(mensagem)){
    length = sizeof(mensagem) - 1;
  }

  memcpy(mensagem,payload,length); //copia exatamente length bytes do payload 
  mensagem[length] = '\0'; // transforma o array em string C valida

  Serial.printf("[%s] %s\n", topic, mensagem);
  
  bool comandoAchado = false;
  for (int i = 0; i < numComandos; i++) {
    if (strcmp(mensagem, comandos[i].name) == 0){
      irsend.sendNEC(comandos[i].comando, 32);
      comandoAchado = true;
      break;
    }
  }
  if (!comandoAchado) {
    Serial.println("Comando desconhecido");
    return;
  }

  regras(mensagem);
  publicarEstado();
}

bool conectarMqtt(){
  String macAddres = String(WiFi.macAddress());
  macAddres.replace(":","");
  String clientId = "esp32-ventilador-" + macAddres;

  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass, TOPIC_STATUS, 1, true, "offline")){
    Serial.println("MQTT conectado");
    client.publish(TOPIC_STATUS, "online", true);
    client.subscribe(TOPIC_CMD, 1);
    publicarEstado();
    return true;
  }
  Serial.printf("MQTT falhou, rc=%d\n", client.state());
  return false;
}

void setup() {
  Serial.begin(115200);
  irsend.begin();
  pinMode(led_pin,OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println("Conectando no wifi...");
  }
  Serial.println("Conectado ao wifi");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);
  conectarMqtt();
}

void loop() {
  if(WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!client.connected()){
    if(millis() - ultimaTentativa > 5000){
      ultimaTentativa = millis();
      conectarMqtt();
    }
  } else {
    client.loop();
  }
}