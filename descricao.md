

## **Componentes e Bibliotecas**
### Bibliotecas Importadas
- **PubSubClient:** Comunicação via MQTT.
- **SPIFFS:** Armazenamento local no ESP32.
- **NTPClient:** Sincronização de horário com servidores NTP.
- **TimeLib:** Manipulação de data e hora.
- **IRremoteESP8266:** Controle remoto via infravermelho.

---
## **Descrição das Variáveis**

### **Configurações Gerais**
- **`IO_USERNAME` e `IO_KEY`:** Credenciais da **Adafruit IO**.
- **`ssid` e `password`:** Credenciais da rede Wi-Fi.
- **`mqttserver`, `mqttport`, `mqttUser`, `mqttPassword`:** Configurações para o cliente MQTT.

---

### **Tempo e Temporização**
- **`previousMillis` e `interval`:** Controle de periodicidade no envio de dados (1 minuto).
- **`ntpServer` e `timeStampUTC`:** Configurações do servidor NTP e fuso horário.

---

### **Infrared (IR)**
- **`kIrLedPin`:** Define o pino do emissor IR.
- **Códigos IR:** 
  - `LIGAR`, `DESLIGAR`, `AUMENTAR`, `TEMP_22`, `TEMP_18`, `PROJ`: Códigos hexadecimais para controlar o ar-condicionado.

---

### **Sensores e Contagem**
- **`sensor1[]` e `sensor2[]`:** Configuração dos pinos dos sensores ultrassônicos.
- **`sensor1Initial`, `sensor2Initial`:** Distâncias iniciais para calibrar os sensores.
- **`currentPeople`:** Contagem atual de pessoas no ambiente.
- **`sequence`:** Armazena a ordem de ativação dos sensores para identificar entrada ou saída.
- **`doorBlocked`:** Impede múltiplas contagens quando a porta está bloqueada.

---

### **Controle de Temperatura**
- **`currentTemperature`:** Temperatura atual do ar-condicionado.
- **Flags:** 
  - `wasAboveFive`: Indica se a contagem já passou de 5 pessoas.
  - `wasAboveFifteen`: Indica se a contagem já passou de 15 pessoas.

---

## **Descrição das Funções**

### **Funções de Conexão e Comunicação**
#### `setup_wifi()`

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
Estabelece a conexão com a rede Wi-Fi utilizando as credenciais definidas. Exibe o status da conexão e o endereço IP no monitor serial.

#### `reconnect()`
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
Verifica se o cliente MQTT está conectado. Em caso de desconexão, tenta reconectar com o servidor MQTT da **Adafruit IO**, exibindo mensagens no monitor serial sobre o progresso e o estado da conexão.

---

### **Funções de Controle IR**
#### `sendIRCommand(uint32_t command)`
```cpp
    void sendIRCommand(uint32_t command) {
  irsend.sendNEC(command, 32); // Envia comando no protocolo NEC com 32 bits
  Serial.print("Comando IR enviado: 0x");
  Serial.println(command, HEX);
}
```
Envia comandos infravermelhos para o ar-condicionado utilizando o protocolo NEC. O comando a ser enviado é especificado em hexadecimal. Essa função é usada para ligar, desligar ou ajustar a temperatura do ar.

---

### **Funções de Sensores**
#### `measureDistance(int a[])`
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
Mede a distância usando sensores ultrassônicos. Configura o pino como saída, emite um pulso ultrassônico e mede o tempo até o retorno do eco. A distância é calculada com base no tempo do pulso.

---

### **Funções de Log no SPIFFS**
#### `enviarSPIFFS()`
```cpp
void enviarSPIFFS() {
  // Atualizar o cliente de tempo para obter a hora atual
  timeClient.update();

  // Obter hora e data formatadas
  String timeString = timeClient.getFormattedTime();
  time_t rawTime = timeClient.getEpochTime();
  struct tm *ptm = localtime(&rawTime);
  String dateString = String(ptm->tm_mday) + "-" + String(ptm->tm_mon + 1) + "-" + String(ptm->tm_year + 1900);

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

```
Atualiza o registro local no SPIFFS com a contagem atual de pessoas. Obtém a data e hora atual do servidor NTP, formata os dados e os armazena em um arquivo chamado `people_log.txt`.

#### `lerArquivoSPIFFS(const char* caminho)`
```cpp
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
```
Lê o conteúdo de um arquivo específico no SPIFFS e exibe no monitor serial. Útil para verificar os logs armazenados.

---

### **Funções de Controle de Temperatura**
#### `ajustAR()`
```cpp
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
```
Ajusta a operação do ar-condicionado com base na contagem atual de pessoas:
- Se **0 pessoas**, desliga o ar-condicionado e apaga todos os LEDs indicadores.
- Se tiver pelo menos **1 pessoa** o ar-condicionado ligará e acenderá o LED verde.
- Se entre **5 e 14 pessoas**, ajusta para 22°C e acende o LED vermelho.
- Se **15 ou mais pessoas**, ajusta para 18°C e acende o LED amarelo.

A função utiliza variáveis de estado para evitar reenvio desnecessário de comandos IR.

---

### **Funções de Envio de Dados**
#### `sendPeopleCount()`
```cpp
void sendPeopleCount() {
  char peopleCountStr[8];
  snprintf(peopleCountStr, sizeof(peopleCountStr), "%d", currentPeople);
  if (client.publish("jeanmagnus/feeds/people-count", peopleCountStr)) {
    Serial.println("Contagem de pessoas enviada: " + String(currentPeople));
  } else {
    Serial.println("Falha ao enviar contagem de pessoas!");
  }
}
```
Envia a contagem atual de pessoas para o feed MQTT correspondente na **Adafruit IO**. Exibe mensagens no monitor serial para confirmar o envio ou relatar falhas.

#### `sendTemperature()`
```cpp
void sendTemperature() {
  char temperatureStr[8];
  snprintf(temperatureStr, sizeof(temperatureStr), "%d", currentTemperature);
  if (client.publish("jeanmagnus/feeds/temperature", temperatureStr)) {
    Serial.println("Temperatura enviada: " + String(currentTemperature) + "ºC");
  } else {
    Serial.println("Falha ao enviar temperatura!");
  }
}
```
Envia a temperatura atual para o feed MQTT correspondente na **Adafruit IO**, semelhante à função anterior.

---

### **Loop Principal**
O loop principal realiza as seguintes tarefas:
1. Verifica e mantém a conexão com o MQTT.
2. Lê os valores dos sensores ultrassônicos.
3. Atualiza a contagem de pessoas com base na sequência de acionamento dos sensores (entrada ou saída).
4. Ajusta a temperatura do ar-condicionado de acordo com a contagem de pessoas.
5. Envia os dados para o SPIFFS e para a **Adafruit IO** periodicamente.

---

## **Fluxo do Projeto**
1. **Configuração Inicial:** O ESP32 conecta-se à rede Wi-Fi e ao servidor MQTT.
2. **Leitura de Sensores:** Sensores ultrassônicos detectam movimento.
3. **Contagem de Pessoas:** Baseado na ordem de ativação dos sensores, incrementa ou decrementa a contagem.
4. **Ajuste do Ar-Condicionado:** Envia comandos IR para o ar com base na contagem.
5. **Registro de Dados:** Salva informações no SPIFFS e envia para o MQTT.

---

