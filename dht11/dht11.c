/*
 * MIT License
 * 
 * Copyright (c) 2018 Michele Biondi
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "esp_timer.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"

#include "dht11.h"

/*static gpio_num_t dht_gpio;
static int64_t last_read_time = -2000000;
static struct dht11_reading last_read;*/

typedef struct
{
    gpio_num_t dht_gpio;
    int64_t last_read_time;
    struct dht11_reading last_read;
} dht_arr_t;

dht_arr_t* dht_arr;

static int _waitOrTimeout(uint16_t microSeconds, int level, uint8_t select) {
    int micros_ticks = 0;
    while(gpio_get_level(dht_arr[select].dht_gpio) == level) { 
        if(micros_ticks++ > microSeconds) 
            return DHT11_TIMEOUT_ERROR;
        ets_delay_us(1);
    }
    return micros_ticks;
}

static int _checkCRC(uint8_t data[]) {
    if(data[4] == (data[0] + data[1] + data[2] + data[3]))
        return DHT11_OK;
    else
        return DHT11_CRC_ERROR;
}

static void _sendStartSignal(uint8_t select) {
    gpio_set_direction(dht_arr[select].dht_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_arr[select].dht_gpio, 0);
    ets_delay_us(20 * 1000);
    gpio_set_level(dht_arr[select].dht_gpio, 1);
    ets_delay_us(40);
    gpio_set_direction(dht_arr[select].dht_gpio, GPIO_MODE_INPUT);
}

static int _checkResponse(uint8_t select) {
    /* Wait for next step ~80us*/
    if(_waitOrTimeout(80, 0, select) == DHT11_TIMEOUT_ERROR)
        return DHT11_TIMEOUT_ERROR;

    /* Wait for next step ~80us*/
    if(_waitOrTimeout(80, 1, select) == DHT11_TIMEOUT_ERROR) 
        return DHT11_TIMEOUT_ERROR;

    return DHT11_OK;
}

static struct dht11_reading _timeoutError() {
    struct dht11_reading timeoutError = {DHT11_TIMEOUT_ERROR, -1, -1};
    return timeoutError;
}

static struct dht11_reading _crcError() {
    struct dht11_reading crcError = {DHT11_CRC_ERROR, -1, -1};
    return crcError;
}

void DHT11_init(gpio_num_t gpio_num[], uint8_t count) {
    /* Wait 1 seconds to make the device pass its initial unstable status */
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    /*dht_gpio = gpio_num;*/
    dht_arr = malloc(count * sizeof(dht_arr_t));
    if(dht_arr == 0)
    {
        printf("dht11 malloc fail\n");
        abort();
    }
    for(uint8_t i = 0; i < count; i++)
        dht_arr[i].dht_gpio = gpio_num[i];
}

struct dht11_reading DHT11_read(uint8_t select) {
    /* Tried to sense too son since last read (dht11 needs ~2 seconds to make a new read) */
    if(esp_timer_get_time() - 2000000 < dht_arr[select].last_read_time) {
        return dht_arr[select].last_read;
    }

    dht_arr[select].last_read_time = esp_timer_get_time();

    uint8_t data[5] = {0,0,0,0,0};

    _sendStartSignal(select);

    if(_checkResponse(select) == DHT11_TIMEOUT_ERROR)
        return dht_arr[select].last_read = _timeoutError();
    
    /* Read response */
    for(int i = 0; i < 40; i++) {
        /* Initial data */
        if(_waitOrTimeout(50, 0, select) == DHT11_TIMEOUT_ERROR)
            return dht_arr[select].last_read = _timeoutError();
                
        if(_waitOrTimeout(70, 1, select) > 28) {
            /* Bit received was a 1 */
            data[i/8] |= (1 << (7-(i%8)));
        }
    }

    if(_checkCRC(data) != DHT11_CRC_ERROR) {
        dht_arr[select].last_read.status = DHT11_OK;
        dht_arr[select].last_read.temperature = data[2];
        dht_arr[select].last_read.humidity = data[0];
        return dht_arr[select].last_read;
    } else {
        return dht_arr[select].last_read = _crcError();
    }
}
