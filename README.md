# Controle de ventilador com ESP32

O ESP32 recebe comandos por MQTT via Wi-Fi e os envia ao ventilador como sinais infravermelhos NEC de 32 bits. Os códigos IR usados pelo firmware estão em `sketch_oct8a.ino`.

## Requisitos

- ESP32 e um emissor infravermelho com sinal no GPIO 18, apontado para o receptor do ventilador.
- Arduino IDE com suporte a placas ESP32 e as bibliotecas `PubSubClient` e `IRremoteESP8266`.
- Rede Wi-Fi e acesso a um broker MQTT.

## Configuração

1. Copie `secrets.example.h` para `secrets.h`.
2. Preencha `ssid` e `password` com os dados do Wi-Fi. Configure `mqtt_server`, substitua `xxxx` em `mqtt_port` pelo número da porta do broker e preencha `mqtt_user` e `mqtt_pass` com as credenciais do MQTT. O Git ignora `secrets.h`.
3. Abra `sketch_oct8a.ino` na Arduino IDE, selecione a placa e a porta serial e carregue o sketch. Use o Monitor Serial a 115200 baud para acompanhar a conexão e os comandos recebidos.

## Uso via MQTT

| Tópico | Conteúdo |
| --- | --- |
| `casa/quarto/ventilador/comando` | Publique o nome exato do comando como texto simples, por exemplo `velocidade_2`. |
| `casa/quarto/ventilador/estado` | JSON retido com os campos `sistema`, `luz` e `velocidade` (0 a 3). |
| `casa/quarto/ventilador/status` | Disponibilidade retida. O ESP32 publica `online` ao conectar e o broker publica `offline` se a conexão cair. |

- Alimentação e ventilação — `power`, `ventilador`, `velocidade_1`, `velocidade_2`, `velocidade_3`.
- Iluminação — `luz`, `luz_fraca`, `luz_laranja`, `luz_amarela`, `luz_branca`, `menos_claridade`, `mais_claridade`, `grd_amarela`, `grd_branca`.
- Temporizador — `timer_1h`, `timer_2h`, `timer_4h`.

O estado publicado é uma estimativa mantida em memória com base nos comandos recebidos. Ele é reiniciado quando o ESP32 reinicia e pode divergir do estado físico após o uso do controle remoto ou se o sinal IR não chegar ao ventilador.
