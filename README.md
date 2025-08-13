# Estação Meteorológica utilizando o Módulo RFM95W para Recebimento e Envio de Dados

**Alunos:** Emyle Santana da Silva, Davi Dantas Mendez Sanchez e Juan Pablo Azevedo Sousa  
 

---

## Objetivo Geral
Desenvolver um sistema capaz de enviar dados de uma estação meteorológica para um segundo dispositivo utilizando comunicação **LoRa** por meio do módulo **RFM95W**.

---

## Descrição Funcional
O sistema opera em dois modos principais:

### **Modo TX (Transmissor)**
- Coleta dados da estação meteorológica utilizando:
  - **AHT20** → Umidade.
  - **BMP280** → Temperatura e pressão.
- Normaliza e armazena os valores em um buffer.
- Atualiza o **display OLED** com os novos dados.
- Envia as informações através do módulo **RFM95W** utilizando LoRa.

### **Modo RX (Receptor)**
- Monitora constantemente o recebimento de dados via **RFM95W**.
- Verifica se os dados recebidos estão no formato correto.
- Exibe as informações no **display OLED** da placa receptora.

---

## Estrutura do Código
- **`/lib/sensors`**  
  Contém as funções de cada sensor (AHT20 e BMP280) em arquivos `.h` e `.c` separados.
- **`/lib/rfm95`**  
  Biblioteca `rfm95.h` e `rfm95.c` com funções para:
  - Configuração do módulo LoRa.
  - Definição do modo de operação (TX/RX).
  - Ajuste dos parâmetros LoRa.
  - Configuração de payloads.

---

## Uso dos Periféricos da BitDogLab
| Componente           | Função | Protocolo / Porta |
|----------------------|--------|-------------------|
| **Display OLED SSD1306** | Exibe temperatura, pressão e umidade | I2C (i2c1) |
| **BMP280**           | Lê temperatura e pressão | I2C (i2c0, via extensão) |
| **AHT20**            | Lê umidade | I2C (i2c0, via extensão) |
| **RFM95W**           | Transmissão e recepção LoRa | SPI |

> O uso de um **extensor I2C** foi necessário para conectar simultaneamente os sensores e o display OLED, devido à limitação de pinos I2C na BitDogLab.

---

## ⚙️ Instalação e Uso

1. **Pré-requisitos**
   - Clonar o repositório:
     ```bash
     git clone https://github.com/JotaPablo/TarefaLoRa.git
     cd TarefaLoRa
     ```
   - Instalar no VS Code:
     - **C/C++**
     - **Pico SDK**
     - **CMake Tools**
     - **Compilador ARM GCC**
  
2. **Alterando o Modo de Operação**
   
   No arquivo **`Lora.c`**, localize a função `main` e altere a seguinte linha para definir o modo desejado:
   
    ```c
    int modo_sistema = MODO_TX;
    ```

3. **Compilação**
   - Crie a pasta de build e compile:
     ```bash
     mkdir build
     cd build
     cmake ..
     make
     ```
   - Ou use o botão “Build” do VS Code com a extensão Raspberry Pi Pico.

4. **Execução**
   - Conecte o Pico segurando o botão BOOTSEL.
   - Copie o `.uf2` gerado na pasta `build` para o drive `RPI-RP2`.

---
  
