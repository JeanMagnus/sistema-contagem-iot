# Sistema de Contagem de Pessoas e Controle de Climatização com ESP32 👥


## Integrantes

* Jean Magnus
* Thyana Maria

## Sumário

 1. [Código do projeto](/code/code.c)
 2. [Descrição do código](/descricao.md)
 3. [Vídeo de simulação]()

## Introdução
Este projeto implementa um sistema de contagem de pessoas e controle automatizado de um ar-condicionado utilizando um ESP32. A contagem é feita com sensores ultrassônicos, e os dados são enviados para o **Adafruit IO** via MQTT. O sistema ajusta a temperatura do ar-condicionado com base no número de pessoas presentes na sala. Além disso, o código registra logs de atividades no sistema de arquivos SPIFFS.


---


## Objetivos
- **Monitorar ocupação de uma sala:** Usar sensores ultrassônicos para detectar entradas e saídas.
- **Controlar a climatização:** Ajustar a temperatura do ar-condicionado de acordo com a quantidade de pessoas presentes.
- **Registrar dados:** Armazenar logs de atividades, incluindo hora e data, no sistema de arquivos SPIFFS.
- **Integrar com Adafruit IO:** Publicar os dados de contagem de pessoas e temperatura na plataforma.


---

