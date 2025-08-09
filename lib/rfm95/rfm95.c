#include "rfm95.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>
#include <stdbool.h>


// Definições pinos SPI e controle
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_RST  20

// Funções auxiliares para controlar CS
static inline void cs_select(void) {
    gpio_put(PIN_CS, 0);
}
static inline void cs_deselect(void) {
    gpio_put(PIN_CS, 1);
}

// Escrever no registrador do SX1276
static void rfm95_write_register(uint8_t reg, uint8_t data) {
    uint8_t buf[2];
    buf[0] = reg | 0x80; // bit 7 = 1 para escrita
    buf[1] = data;

    cs_select();
    spi_write_blocking(SPI_PORT, buf, 2);
    cs_deselect();

    sleep_ms(1);
}

// Ler registrador do SX1276
static uint8_t rfm95_read_register(uint8_t addr) {
    uint8_t buf[1];
    addr &= 0x7F; // bit 7 = 0 para leitura

    cs_select();
    spi_write_blocking(SPI_PORT, &addr, 1);
    sleep_ms(1);
    spi_read_blocking(SPI_PORT, 0, buf, 1);
    cs_deselect();

    sleep_ms(1);

    //printf("READ %02X\n", buf[0]);
    return buf[0];
}

// Lê um bloco de dados a partir da FIFO 
static void rfm95_read_fifo(uint8_t* buffer, uint8_t length) {
    uint8_t addr = REG_FIFO & 0x7F;
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, &addr, 1);
    spi_read_blocking(SPI_PORT, 0, buffer, length);
    gpio_put(PIN_CS, 1);
}

// Escreve um bloco de dados na FIFO 
static void rfm95_write_fifo(const uint8_t* buffer, uint8_t length) {
    uint8_t addr = REG_FIFO | 0x80;
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, &addr, 1);
    spi_write_blocking(SPI_PORT, buffer, length);
    gpio_put(PIN_CS, 1);
}

// Reinicia o módulo forçando o pino RST
static void rfm95_reset() {
    gpio_put(PIN_RST, 0);
    sleep_ms(10);
    gpio_put(PIN_RST, 1);
    sleep_ms(10);
}


// Modos
void rfm95_sleep_mode(){
    rfm95_write_register(REG_OPMODE, RF95_MODE_SLEEP);
}

void rfm95_standby_mode(){
    rfm95_write_register(REG_OPMODE, RF95_MODE_STANDBY);
}

void rfm95_tx_mode(){
    rfm95_write_register(REG_OPMODE, RF95_MODE_TX);
}

void rfm95_rx_mode(){
    rfm95_write_register(REG_OPMODE, RF95_MODE_RX_CONTINUOUS);
}

// Define frequência de operação (em MHz)
void rfm95_set_frequency(double Frequency) {
    unsigned long FrequencyValue;
    //printf("Frequência escolhida %.3f MHz\n", Frequency);
    Frequency = Frequency * 7110656 / 434; // fórmula do material (base 434 MHz)
    FrequencyValue = (unsigned long)(Frequency);
    //printf("Frequencia calculada para ajuste dos registradores: %lu\n", FrequencyValue);

    rfm95_write_register(REG_FRF_MSB, (FrequencyValue >> 16) & 0xFF);
    rfm95_write_register(REG_FRF_MID, (FrequencyValue >> 8) & 0xFF);
    rfm95_write_register(REG_FRF_LSB, FrequencyValue & 0xFF);

    /*
    printf("Impressão de valor binário da frequência:\n");
    imprimir_binario(FrequencyValue);
    printf("\n");
    printf("Frequ. Regs. config:\n");

    // Testa leitura dos registradores
    readRegister(REG_FRF_MSB);
    readRegister(REG_FRF_MID);
    readRegister(REG_FRF_LSB);
    */
}

bool rfm95_init(){

    // Inicializa a SPI
    spi_init(SPI_PORT, 1000000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);

    // Reseta o módulo
    rfm95_reset();

    // Modo sleep
    rfm95_sleep_mode();

    // Frequência
    rfm95_set_frequency(915.0f);

    // Configurar Modem (BW=125kHz, CR=4/5, SF=7)
    rfm95_write_register(REG_MODEM_CONFIG, BANDWIDTH_125K | ERROR_CODING_4_5 | EXPLICIT_MODE); 
    rfm95_write_register(REG_MODEM_CONFIG2, SPREADING_7 | CRC_ON); 
    rfm95_write_register(REG_MODEM_CONFIG3, 0x04); // LowDataRateOptimize=off, AGC=on

    // Perambulo
    rfm95_write_register(REG_PREAMBLE_MSB, 0x00);
    rfm95_write_register(REG_PREAMBLE_LSB, 0x08);

    // Define o payload
    rfm95_write_register(REG_PAYLOAD_LENGTH, 255);

    // Configurar endereços base do FIFO
    rfm95_write_register(REG_FIFO_TX_BASE_AD, 0x00);
    rfm95_write_register(REG_FIFO_RX_BASE_AD, 0x00);

    // Coloca em modo standby novamente
    rfm95_standby_mode();

    return true;
}

void rfm95_send_data(const char *msg){

    rfm95_standby_mode();

    uint8_t length = strlen(msg);
    if (length > PAYLOAD_LENGTH) length = PAYLOAD_LENGTH;

    rfm95_write_register(REG_FIFO_ADDR_PTR, 0x00);
    
    // Escrever dados no FIFO
    rfm95_write_fifo(msg, length);

    rfm95_write_register(REG_PAYLOAD_LENGTH, length);

    rfm95_tx_mode();

    while(!(rmf95_read_register(REG_IRQ_FLAGS) & 0x08)) {
        sleep_ms(1);                                    
    }

    // Limpar flag TxDone
    rfm95_write_register(REG_IRQ_FLAGS, 0x08);
    
    // Voltar para standby
    rfm95_standby_mode();


}

bool rfm95_receive_data(char *msg, int max_length){

    if(rfm95_read_register(REG_IRQ_FLAGS) & 0x40){

        uint8_t length = rfm95_read_register(REG_RX_NB_BYTES);

        if(length > max_length) length = max_length;

        uint8_t fifo_addr = rfm95_read_register(REG_FIFO_RX_CURRENT_ADDR);
        rfm95_write_register(REG_FIFO_ADDR_PTR, fifo_addr);

        rfm95_read_fifo(msg, length);
        msg[length] = '\0';

        // Limpar flags
        rfm95_write_register(REG_IRQ_FLAGS, 0x40);

        return true;
    }

    return false;

}