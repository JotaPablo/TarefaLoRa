#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"

#include "lib/ssd1306/ssd1306.h"
#include "lib/ssd1306/display.h"
#include "lib/led/led.h"
#include "lib/button/button.h"
#include "lib/matrix_leds/neopixel.h"
#include "lib/buzzer/buzzer.h"
#include "lib/sensors/ahto20/aht20.h"
#include "lib/sensors/bmp280/bmp280.h"
#include "lib/rfm95/rfm95.h"

// ==== Configurações ====
#define I2C_PORT_SENSORS i2c0
#define I2C_SDA_SENSORS_PIN 0
#define I2C_SCL_SENSORS_PIN 1

#define MODO_TX 0
#define MODO_RX 1

// ==== Variáveis Globais ====
static ssd1306_t ssd;
volatile uint32_t last_time_debounce_button_a = 0;
volatile uint32_t last_time_debounce_button_b = 0;
volatile uint32_t last_time_debounce_button_sw = 0;
const uint32_t delay_debounce = 200;

volatile float temperatura = 0;
volatile float pressao = 0;
volatile float umidade = 0;
struct bmp280_calib_param bmp_params;

// ==== Botões ====
static void gpio_button_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if (gpio == BUTTON_A && (current_time - last_time_debounce_button_a > delay_debounce)) {
        printf("\nBOTAO A PRESSIONADO\n");
        last_time_debounce_button_a = current_time;
    }
    else if (gpio == BUTTON_B && (current_time - last_time_debounce_button_b > delay_debounce)) {
        printf("\nBOTAO B PRESSIONADO\n");
        last_time_debounce_button_b = current_time;
    }
    else if (gpio == BUTTON_SW) {
        // Aqui poderia colocar código para entrar em modo de gravação
    }
}

// ==== Setup ====
void setup() {
    stdio_init_all();

    init_buzzer(BUZZER_A_PIN, 4.0f);
    init_leds();

    init_display(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    npInit(LED_PIN);
    npWrite();

    i2c_init(I2C_PORT_SENSORS, 400 * 1000);
    gpio_set_function(I2C_SDA_SENSORS_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_SENSORS_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_SENSORS_PIN);
    gpio_pull_up(I2C_SCL_SENSORS_PIN);

    bmp280_init(I2C_PORT_SENSORS);
    bmp280_get_calib_params(I2C_PORT_SENSORS, &bmp_params);

    aht20_reset(I2C_PORT_SENSORS);
    aht20_init(I2C_PORT_SENSORS);

    rfm95_init();

    button_init_predefined(true, true, true);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_SW, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
}

// ==== Funções auxiliares ====
void ler_sensores() {
    int32_t raw_temp_bmp, raw_pressure;
    AHT20_Data aht_data;

    bmp280_read_raw(I2C_PORT_SENSORS, &raw_temp_bmp, &raw_pressure);
    temperatura = bmp280_convert_temp(raw_temp_bmp, &bmp_params) / 100.0f;
    pressao = bmp280_convert_pressure(raw_pressure, raw_temp_bmp, &bmp_params) / 100.0f;

    if (!aht20_read(I2C_PORT_SENSORS, &aht_data)) {
        printf("Erro na leitura do AHT20!\n");
        umidade = 0.0f;
    } else {
        umidade = aht_data.humidity;
    }
}

void atualiza_display() {
    char tempLine[30];
    char humidityLine[30];
    char pressureLine[30];

    snprintf(tempLine, sizeof(tempLine), "Temp: %.1f C", temperatura);
    snprintf(humidityLine, sizeof(humidityLine), "Umidade: %.1f %%", umidade);
    snprintf(pressureLine, sizeof(pressureLine), "Pressao: %.1f hPa", pressao);

    ssd1306_fill(&ssd, false);
    draw_centered_text(&ssd, "Estacao Meteor.", 0);
    ssd1306_draw_string(&ssd, tempLine, 0, 12);
    ssd1306_draw_string(&ssd, humidityLine, 0, 38);
    ssd1306_draw_string(&ssd, pressureLine, 0, 48);
    ssd1306_send_data(&ssd);
}

bool rfm95_available(void) {
    uint8_t irq_flags = rfm95_read_register(REG_IRQ_FLAGS);
    return (irq_flags & 0x40) != 0;
}

void processar_dados_recebidos(const char *dados) {
    // Espera formato: "Temp:xx;Press:yy;Umidade:zz"
    sscanf(dados, "Temp: %f;Press: %f;Umidade: %f", &temperatura, &pressao, &umidade);
}

int main() {
    setup();

    int modo_sistema = MODO_TX;
    char buffer[255];

    // Começa já no modo RX para escutar caso queira
    rfm95_rx_mode();

    while (true) {
        if (modo_sistema == MODO_TX) {
            ler_sensores();
            snprintf(buffer, sizeof(buffer), "Temp: %.2f;Press: %.2f;Umidade: %.2f", temperatura, pressao, umidade);
            atualiza_display();
            rfm95_send_data(buffer);
            //rfm95_receive_data(buffer, 255);
            printf("Oq tá no fifo: %s\n", buffer);
           // rfm95_rx_mode(); 
        }
        else {
            if (rfm95_available()) {
                if (rfm95_receive_data(buffer, 255)){
                    processar_dados_recebidos(buffer);
                    atualiza_display();
                }
            } 
        }

        sleep_ms(200);
    }
}