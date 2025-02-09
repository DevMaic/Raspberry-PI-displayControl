#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pio_matrix.pio.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "frames.c"
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

uint32_t lastTime = 0;
ssd1306_t ssd; // Inicializa a estrutura do display
PIO pio;
uint sm;
char c;

//rotina da interrupção
static void gpio_irq_handler(uint gpio, uint32_t events) {
  uint32_t currentTime = to_us_since_boot(get_absolute_time());
   
  if(currentTime - lastTime > 300000) {
    if (gpio == 5) {
      gpio_put(11, !gpio_get(11));
      printf("Botão A pressionado\n");
      ssd1306_draw_string(&ssd, gpio_get(11)?"LED VERDE ON!":"LED VERDE OFF!", 8, 10);
      ssd1306_send_data(&ssd);
    } else if (gpio == 6) {
      gpio_put(12, !gpio_get(12));
      printf("Botão B pressionado\n");
      ssd1306_draw_string(&ssd, gpio_get(12)?"LED AZUL ON!":"LED AZUL OFF!", 8, 10);
      ssd1306_send_data(&ssd);
    }
    lastTime = currentTime;
  }
}

//rotina para definição da intensidade de cores do led
uint32_t matrix_rgb(double b, double r, double g) {
    unsigned char R, G, B;
    R = r * 255;
    G = g * 255;
    B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

//rotina para acionar a matrix de leds - ws2812b
void desenho_pio(double *desenho, PIO pio, uint sm) {
    uint32_t valor_led;

    for (int16_t i = 0; i < 25; i++) {
        valor_led = matrix_rgb(desenho[24-i], 0, 0);
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

void drawOnLedMatrix(int c) {
  desenho_pio(numeros[c-'0'], pio, sm);
}

void drawOnDisplay(int c) {
  ssd1306_draw_char(&ssd, c, 8, 10);
  ssd1306_send_data(&ssd);
}


int main() {
  stdio_init_all(); // Inicializa a comunicação serial
  gpio_init(5);
  gpio_set_dir(5, GPIO_IN);
  gpio_pull_up(5);

  gpio_init(6);
  gpio_set_dir(6, GPIO_IN);
  gpio_pull_up(6);

  gpio_init(11);
  gpio_set_dir(11, GPIO_OUT);

  gpio_init(12);
  gpio_set_dir(12, GPIO_OUT);

  i2c_init(I2C_PORT, 400 * 1000); // I2C Initialisation. Using it at 400Khz.
  set_sys_clock_khz(128000, false);

  pio = pio0;
  uint offset = pio_add_program(pio, &pio_matrix_program);
  sm = pio_claim_unused_sm(pio, true);
  pio_matrix_program_init(pio, sm, offset, 7);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA); // Pull up the data line
  gpio_pull_up(I2C_SCL); // Pull up the clock line
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_send_data(&ssd); // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  gpio_set_irq_enabled_with_callback(5, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);
  gpio_set_irq_enabled_with_callback(6, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);

  bool cor = true;
  while(true) {
    if(stdio_usb_connected()) {
      if(scanf(" %c", &c) == 1) {
        if(c >= '0' && c <= '9') {
          drawOnLedMatrix(c);
        } else if(c >= 'A' && c <= 'Z') {
          drawOnDisplay(c);
        }
        printf("Caractere lido: %c\n", c);
        // ssd1306_draw_char(&ssd, c, 8, 10);
        // ssd1306_send_data(&ssd);
      }
    }
    cor = !cor;
    // Atualiza o conteúdo do display com animações
    // ssd1306_fill(&ssd, !cor); // Limpa o display
    // ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
    // ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 10); // Desenha uma string
    // ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 30); // Desenha uma string
    // ssd1306_draw_string(&ssd, "PROF WILTON", 15, 48); // Desenha uma string      
    // ssd1306_send_data(&ssd); // Atualiza o display

    sleep_ms(1000);
  }
}