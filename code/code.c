#include <PubSubClient.h>
#include <SPIFFS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>


//---------------------------------------------------
// Configurações de WiFi e Adafruit IO
#define IO_USERNAME  ""
#define IO_KEY       ""
const char* ssid = "";
const char* password = "";


const char* mqttserver = "io.adafruit.com";
const int mqttport = 1883;
const char* mqttUser = IO_USERNAME;
const char* mqttPassword = IO_KEY;
unsigned long previousMillis = 0;  // Armazena o último momento em que a tarefa foi executada
const unsigned long interval = 60000;  // Intervalo de 1 minuto (em milissegundos)


WiFiClient espClient;
PubSubClient client(espClient);
//const char* ntpServer = "pool.ntp.br";
//usando ntp org:
const char* ntpServer = "pool.ntp.org";


const long timeStampUTC = -10800;




WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, timeStampUTC);




//---------------------------------------------------
// Configuração do IR e Sensores
const uint16_t kIrLedPin = 21; // Pino para o LED emissor IR
IRsend irsend(kIrLedPin); // Inicializa o emissor IR


// Códigos IR (hexadecimais) fornecidos
#define LIGAR 0xB2BF00
#define DESLIGAR 0xB27BE0
#define AUMENTAR 0xB2BF10
#define TEMP_22 0xB2BF70
#define TEMP_18 0xB2BF10
#define PROJ 0x807F42BD


// Configuração de sensores e contagem
int currentPeople = 0; // Contagem atual de pessoas
int currentTemperature = 0; // 0 significa ar desligado
int sensor1[] = {4, 5};
int sensor2[] = {12, 13};
int sensor1Initial = 50; // Valor fixo inicial
int sensor2Initial = 50; // Valor fixo inicial
String sequence = "";
int timeoutCounter = 0;
bool doorBlocked = false; // Indica se a porta está bloqueada




//---------------------------------------------------
// Funções auxiliares
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando-se ao WiFi: ");
  Serial.println(ssid);


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  Serial.println("\nWiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());


 
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    String clientId = "ESP32-PeopleCounter";
    clientId += String(random(0xffff), HEX);


    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("Conectado ao Adafruit IO!");
      // Publicar inicialização no feed de contagem
      client.publish("jeanmagnus/feeds/people-count", "Iniciando...");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5s...");
      delay(5000);
    }
  }
}


void sendIRCommand(uint32_t command) {
  irsend.sendNEC(command, 32); // Envia comando no protocolo NEC com 32 bits
  Serial.print("Comando IR enviado: 0x");
  Serial.println(command, HEX);
}


void sendPeopleCount() {
  char peopleCountStr[8];
  snprintf(peopleCountStr, sizeof(peopleCountStr), "%d", currentPeople);
  if (client.publish("jeanmagnus/feeds/people-count", peopleCountStr)) {
    Serial.println("Contagem de pessoas enviada: " + String(currentPeople));
  } else {
    Serial.println("Falha ao enviar contagem de pessoas!");
  }
}


void sendTemperature() {
  char temperatureStr[8];
  snprintf(temperatureStr, sizeof(temperatureStr), "%d", currentTemperature);
  if (client.publish("jeanmagnus/feeds/temperature", temperatureStr)) {
    Serial.println("Temperatura enviada: " + String(currentTemperature) + "ºC");
  } else {
    Serial.println("Falha ao enviar temperatura!");
  }
}


// Função para medir a distância usando o sensor ultrassônico
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


void enviarSPIFFS() {
  // Atualizar o cliente de tempo para obter a hora atual
  timeClient.update();
 // time_t t = now();


  // Obter hora e data formatadas
  String timeString = timeClient.getFormattedTime();
  time_t rawTime = timeClient.getEpochTime();
  struct tm *ptm = localtime(&rawTime);
  String dateString = String(ptm->tm_mday) + "-" + String(ptm->tm_mon + 1) + "-" + String(ptm->tm_year + 1900);


  //String dateString = String(day()) + "-" + String(month()) + "-" + String(year());


  // Abrir ou criar o arquivo no SPIFFS
  File logFile = SPIFFS.open("/people_log.txt", "a");
  if (!logFile) {
    Serial.println("Falha ao abrir o arquivo para escrita!");
    return;
  }


  // Formatar a linha de registro
  String logEntry = dateString + " " + timeString + " | Pessoas: " + String(currentPeople) + "\n";


  // Escrever no arquivo
  logFile.print(logEntry);


  // Fechar o arquivo
  logFile.close();
  Serial.println("Log atualizado no SPIFFS: " + logEntry);
}
void lerArquivoSPIFFS(const char* caminho) {
  // Abrir o arquivo no modo leitura
  File arquivo = SPIFFS.open(caminho, "r");
  if (!arquivo) {
    Serial.print("Falha ao abrir o arquivo: ");
    Serial.println(caminho);
    return;
  }


  Serial.print("Conteúdo do arquivo ");
  Serial.println(caminho);
 
  // Ler e imprimir o conteúdo no monitor serial
  while (arquivo.available()) {
    Serial.write(arquivo.read());
  }
 
  arquivo.close();
}


void ajustAR() {
  static bool wasAboveFive = false;  // Rastrea se estava acima ou igual a 5
  static bool wasAboveFifteen = false; // Rastrea se estava acima ou igual a 15


  if (currentPeople == 0) {
    // DESLIGAR AR
    sendIRCommand(DESLIGAR); // Desligar o ar
    digitalWrite(14, LOW);   // LED Verde desligado
    digitalWrite(27, LOW);   // LED Vermelho desligado
    digitalWrite(26, LOW);   // LED Amarelo desligado
    currentTemperature = 0; // Ar condicionado desligado
    sendTemperature();
    wasAboveFive = false;
    wasAboveFifteen = false;
  } else {
    // LIGAR AR
    if (currentTemperature == 0) {
      sendIRCommand(LIGAR); // Ligar o ar
      sendIRCommand(PROJ);
      currentTemperature = 24; // Temperatura padrão ao ligar
      sendTemperature();
    }
    digitalWrite(14, HIGH); // LED Verde ligado


    // Ajuste para 15 ou mais pessoas
    if (currentPeople >= 15) {
      if (!wasAboveFifteen) {
        sendIRCommand(TEMP_18);      // Ajustar para 18 graus
        digitalWrite(26, HIGH);      // LED Amarelo ligado
        currentTemperature = 18;
        sendTemperature();
        wasAboveFifteen = true;
      }
    } else if (wasAboveFifteen) {
      // Desliga LED Amarelo e envia 0 graus ao Adafruit quando cair abaixo de 15
      digitalWrite(26, LOW);
      currentTemperature = 0;
      sendTemperature();
      wasAboveFifteen = false;
    }


    // Ajuste para 5 ou mais pessoas, mas menos de 15
    if (currentPeople >= 5 && currentPeople < 15) {
      if (!wasAboveFive) {
        sendIRCommand(TEMP_22);      // Ajustar para 22 graus
        digitalWrite(27, HIGH);      // LED Vermelho ligado
        currentTemperature = 22;
        sendTemperature();
        wasAboveFive = true;
      }
    } else if (wasAboveFive && currentPeople < 5) {
      // Desliga LED Vermelho e envia 0 graus ao Adafruit quando cair abaixo de 5
      digitalWrite(27, LOW);
      currentTemperature = 0;
      sendTemperature();
      wasAboveFive = false;
    }
  }
}




//---------------------------------------------------
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqttserver, mqttport);
  reconnect();
  timeClient.begin();
 // timeClient.update();
 // Serial.println("Hora NTP sincronizada");


  // Mostrar la hora actual desde el servidor NTP
  if (!SPIFFS.begin(true)) {
    Serial.println("Falha ao montar SPIFFS");
    return;
  }
  Serial.println("SPIFFS montado com sucesso!");
  lerArquivoSPIFFS("/people_log.txt");


  pinMode(14,OUTPUT); // lED VERDE
  pinMode(27, OUTPUT); // LED VERMELHO
  pinMode(26, OUTPUT); // LED AMARELO


}


//---------------------------------------------------
void loop() {
  unsigned long currentMillis = millis();  // Captura o tempo atual
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  // Leitura dos sensores ultrassônicos
  int sensor1Val = measureDistance(sensor1);
  int sensor2Val = measureDistance(sensor2);


  // Verifica se a porta está bloqueada
  if (sensor1Val >= sensor1Initial && sensor2Val >= sensor2Initial) {
    doorBlocked = false; // Porta liberada
  }


  if (!doorBlocked) {
    // Processamento dos dados
    if (sensor1Val < sensor1Initial && sequence.charAt(0) != '1') {
      sequence += "1";


    } else if (sensor2Val < sensor2Initial && sequence.charAt(0) != '2') {
      sequence += "2";
    }


    if (sequence.equals("12")) {
      currentPeople++;
      //imprimeNTP();
      sequence = "";
      doorBlocked = true;
      delay(550);
      Serial.println("Entrada detectada. Pessoas na sala: " + String(currentPeople));
      sendPeopleCount(); // Envia contagem de pessoas
      ajustAR();
    } else if (sequence.equals("21") && currentPeople > 0) {
      currentPeople--;
      //imprimeNTP();
      sequence = "";
      doorBlocked = true;
      delay(550);
      Serial.println("Saída detectada. Pessoas na sala: " + String(currentPeople));
      sendPeopleCount(); // Envia contagem de pessoas
      ajustAR();
    }


    // Reseta sequência se inválida
    if (sequence.length() > 2 || sequence.equals("11") || sequence.equals("22") || timeoutCounter > 200) {
      sequence = "";
    }


    if (sequence.length() == 1) {
      timeoutCounter++;
    } else {
      timeoutCounter = 0;
    }
  }


    // Verifica se 1 minuto já se passou
  if (currentMillis - previousMillis >= interval) {
    Serial.println("Passou 1 minuto.");
    previousMillis = currentMillis;  // Atualiza o tempo anterior
    enviarSPIFFS();  // Envia os dados ao SPIFFS
  }


 
  // Exibe status atual no terminal
  Serial.print("Sequência: ");
  Serial.print(sequence);
  Serial.print(" | Sensor 1: ");
  Serial.print(sensor1Val);
  Serial.print(" cm | Sensor 2: ");
  Serial.print(sensor2Val);
  Serial.print(" cm | Pessoas na sala: ");
  Serial.println(currentPeople);


  delay(250); // Reduz frequência de leitura
}
