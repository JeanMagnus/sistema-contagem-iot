# Sistema de Contagem de Pessoas e Controle de Climatização com ESP32


## Introdução
Este projeto implementa um sistema de contagem de pessoas e controle automatizado de um ar-condicionado utilizando um ESP32. A contagem é feita com sensores ultrassônicos, e os dados são enviados para o **Adafruit IO** via MQTT. O sistema ajusta a temperatura do ar-condicionado com base no número de pessoas presentes na sala. Além disso, o código registra logs de atividades no sistema de arquivos SPIFFS.


---


## Objetivos
- **Monitorar ocupação de uma sala:** Usar sensores ultrassônicos para detectar entradas e saídas.
- **Controlar a climatização:** Ajustar a temperatura do ar-condicionado de acordo com a quantidade de pessoas presentes.
- **Registrar dados:** Armazenar logs de atividades, incluindo hora e data, no sistema de arquivos SPIFFS.
- **Integrar com Adafruit IO:** Publicar os dados de contagem de pessoas e temperatura na plataforma.


---


## Estrutura do Código


### Configurações Iniciais
```cpp
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
```
**Descrição:**


Importação das bibliotecas necessárias:


- PubSubClient: Para comunicação MQTT.
- SPIFFS: Para armazenamento de logs.
- NTPClient: Para sincronização de tempo via NTP.
- WiFiUdp: Para conexão UDP com o servidor NTP.
- IRremoteESP8266: Para envio de comandos IR ao ar-condicionado.


---
### Configuração de Wi-Fi e MQTT


```cpp
#define IO_USERNAME  "YOUR-IO-USERNAME-HERE"
#define IO_KEY       "YOUR-IO-KEY-HERE"
const char* ssid = "---";
const char* password = "---";
```
Define as credenciais do Wi-Fi e Adafruit IO.


```cpp
const char* mqttserver = "io.adafruit.com";
const int mqttport = 1883;
const char* mqttUser = IO_USERNAME;
const char* mqttPassword = IO_KEY;
```
Configurações para conectar ao servidor MQTT da Adafruit.


---


### Inicialização do Sistema


**Conexão Wi-Fi**


```cpp
void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi conectado!");
}
```
**Descrição:**


Estabelece a conexão com a rede Wi-Fi especificada.


---


### Conexão MQTT


```cpp
void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP32-PeopleCounter";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("Conectado ao Adafruit IO!");
    } else {
      delay(5000);
    }
  }
}
```
**Descrição:**


Tenta reconectar ao servidor MQTT caso a conexão seja perdida.


---


### Sincronização de Hora


```cpp
const char* ntpServer = "pool.ntp.org";
NTPClient timeClient(ntpUDP, ntpServer, -10800);


```


**Descrição:**
Configura o cliente NTP para sincronizar o horário com um servidor.




### Sensores e Controle


**Leitura dos Sensores**


```cpp
int measureDistance(int a[]) {
  pinMode(a[1], OUTPUT);
  digitalWrite(a[1], LOW);
  delayMicroseconds(2);
  digitalWrite(a[1], HIGH);
  delayMicroseconds(10);
  digitalWrite(a[1], LOW);
  pinMode(a[0], INPUT);
  long duration = pulseIn(a[0], HIGH, 100000);
  return duration / 29 / 2;
}


```
**Descrição:**


Realiza a leitura de distância utilizando sensores ultrassônicos.


### Controle do Ar-Condicionado


```cpp
void ajustAR() {
  if (currentPeople == 0) {
    sendIRCommand(DESLIGAR);
    digitalWrite(14, LOW); // LED Verde
    digitalWrite(27, LOW); // LED Vermelho
    digitalWrite(26, LOW); // LED Amarelo
  } else if (currentPeople >= 5 && currentPeople < 15) {
    sendIRCommand(TEMP_22);
    digitalWrite(27, HIGH);
  } else if (currentPeople >= 15) {
    sendIRCommand(TEMP_18);
    digitalWrite(26, HIGH);
  }
}
```
**Descrição:**


Ajusta a temperatura do ar-condicionado com base na quantidade de pessoas.


### Publicação de Dados


```cpp
void sendPeopleCount() {
  char peopleCountStr[8];
  snprintf(peopleCountStr, sizeof(peopleCountStr), "%d", currentPeople);
  client.publish("jeanmagnus/feeds/people-count", peopleCountStr);
}


```


**Descrição:**


Envia a contagem de pessoas para o Adafruit IO.


---


### Logs no SPIFFS


```cpp
void enviarSPIFFS() {
  timeClient.update();
  String timeString = timeClient.getFormattedTime();
  String dateString = String(ptm->tm_mday) + "-" + String(ptm->tm_mon + 1) + "-" + String(ptm->tm_year + 1900);
  File logFile = SPIFFS.open("/people_log.txt", "a");
  String logEntry = dateString + " " + timeString + " | Pessoas: " + String(currentPeople) + "\n";
  logFile.print(logEntry);
  logFile.close();
}


```
**Descrição:**
Registra a contagem de pessoas com a hora e data no sistema de arquivos SPIFFS.




---


### Estrutura do Loop Principal


```cpp
void loop() {
  if (!client.connected()) reconnect();
  client.loop();
  int sensor1Val = measureDistance(sensor1);
  int sensor2Val = measureDistance(sensor2);
  // Lógica para entrada e saída de pessoas
  if (sequence.equals("12")) {
    currentPeople++;
    sendPeopleCount();
    ajustAR();
  } else if (sequence.equals("21") && currentPeople > 0) {
    currentPeople--;
    sendPeopleCount();
    ajustAR();
  }
  // Atualiza logs a cada minuto
  if (millis() - previousMillis >= interval) {
    enviarSPIFFS();
    previousMillis = millis();
  }
}


```
**Descrição:**
Gerencia a contagem de pessoas, ajusta o ar-condicionado e registra os dados.


---
 
 ## Como usar

 1. Configure as credenciais Wi-Fi e Adafruit IO.
 2. Faça upload do código para um ESP32.
3. Verifique os logs no monitor serial para diagnósticos.
4. Acompanhe os dados no painel do Adafruit IO.