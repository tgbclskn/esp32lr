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

//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"

#include "dht11.h"
#include <stdint.h>

#define DHT_COUNT 2

/*static gpio_num_t dht_gpio;
static int64_t last_read_time = -2000000;
static struct dht11_reading last_read;*/

typedef struct
{
    uint8_t dht_gpio;
    int64_t last_read_time;
    struct dht11_reading last_read;
} dht_arr_t;

dht_arr_t* dht_arr;


uint8_t dht_stack[DHT_COUNT * sizeof(dht_arr_t)];


static int _waitOrTimeout(uint16_t microSeconds, int level, uint8_t select) {
    int nondet_micros_ticks;
    __CPROVER_assume( 0 <= nondet_micros_ticks );

    int nondet_x;

    if(nondet_x) return nondet_micros_ticks;
    else return DHT11_TIMEOUT_ERROR;
}

static int _checkCRC(uint8_t data[]) {
    
    int nondet_x;
    if(nondet_x) return DHT11_OK;
    else return DHT11_CRC_ERROR;
}

static void _sendStartSignal(uint8_t select) {}

static int _checkResponse(uint8_t select) {
   
    int nondet_x;
    if(nondet_x) return DHT11_OK;
    else return DHT11_TIMEOUT_ERROR;

}

static struct dht11_reading _timeoutError() {
    struct dht11_reading timeoutError = {DHT11_TIMEOUT_ERROR, -1, -1};
    return timeoutError;
}

static struct dht11_reading _crcError() {
    struct dht11_reading crcError = {DHT11_CRC_ERROR, -1, -1};
    return crcError;
}

void DHT11_init(uint8_t gpio_num[], uint8_t count) {

    __CPROVER_assume( __CPROVER_r_ok( gpio_num, count ) );

    memset(dht_stack, 0, DHT_COUNT * sizeof(dht_arr_t));

    /*dht_arr = malloc(count * sizeof(dht_arr_t));
    if(dht_arr == 0)
    {
        printf("dht11 malloc fail\n");
        abort();
    }*/
    dht_arr = (dht_arr_t*)(&dht_stack[0]);
    for(uint8_t i = 0; i < count && i < DHT_COUNT; i++)
        dht_arr[i].dht_gpio = gpio_num[i];
}

struct dht11_reading DHT11_read(uint8_t select) {

    __CPROVER_assume( select == 0 || select == 1 );

    /* Tried to sense too son since last read (dht11 needs ~2 seconds to make a new read) */
    
    int64_t nondet_cur_time;
    __CPROVER_assume( (0 <= nondet_cur_time) && (nondet_cur_time <= INT32_MAX) );
    
    int nondet_x;
    if(nondet_x) return dht_arr[select].last_read;

    __CPROVER_assume( dht_arr[select].last_read_time < nondet_cur_time );
    dht_arr[select].last_read_time = nondet_cur_time;

    uint8_t data[5] = {0,0,0,0,0};

    if(_checkResponse(select) == DHT11_TIMEOUT_ERROR)
        return dht_arr[select].last_read = _timeoutError();
    
    
    if(_waitOrTimeout(50, 0, select) == DHT11_TIMEOUT_ERROR)
        return dht_arr[select].last_read = _timeoutError();
    
    __CPROVER_havoc_slice(data, 5);

    if(_checkCRC(data) != DHT11_CRC_ERROR) {
        dht_arr[select].last_read.status = DHT11_OK;
        dht_arr[select].last_read.temperature = (data[2] >= 128)? 127 : data[2];
        dht_arr[select].last_read.humidity = (data[0] >= 128)? 127 : data[0];
        return dht_arr[select].last_read;
    } else {
        return dht_arr[select].last_read = _crcError();
    }
}
