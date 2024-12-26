#include <stdio.h>
#include "esp_wifi.h"
#include "driver/i2c_master.h"
#include "pthread.h"
#include "string.h"
#include "lwip/sockets.h"

#define TSC_I2C_ADDR 0x29

#define SENSOR_COUNT 4
typedef struct
{
	uint16_t all[0];
	//uint8_t surface_temp;
	//uint8_t amb_temp;
	//uint8_t humidity;
	uint16_t r;
	uint16_t g;
	uint16_t b;
	uint16_t c;
} sensor_t;

typedef struct
{
	uint32_t all[0];
	uint32_t r;
	uint32_t g;
	uint32_t b;
	uint32_t c;
} sensor_sum_t;

typedef struct
{
	uint32_t ice;
	sensor_t sec_30;
	sensor_t min_1;
	sensor_t min_3;
	sensor_t min_9;
} data_avg_t;

#define RING_BUF_SIZE 540
#define RING_DATA_SIZE 24*60*2
typedef struct
{
	uint32_t front;
	uint32_t back;
	uint32_t full;
	uint32_t empty;
	void* 	 buf;
} ringbuf_t;

typedef struct
{
	ringbuf_t* ring;
	pthread_mutex_t* mutex;
} wifi_sender_arg_t;


static void i2c_add_dev(i2c_master_bus_handle_t, i2c_master_dev_handle_t*, const uint8_t) __attribute__((noinline));
static void i2csend_and_check(i2c_master_dev_handle_t*, const uint8_t) __attribute__((noinline));
static void i2csend(i2c_master_dev_handle_t*, const uint8_t) __attribute__((noinline));
static uint8_t i2cget(i2c_master_dev_handle_t*) __attribute__((noinline));
static void sta_ip_assigned_handler(void*, esp_event_base_t, int32_t, void*);
static void ring_increment(ringbuf_t*, pthread_mutex_t*, const uint32_t) __attribute__((noinline));


void app_main(void)
{
    i2c_master_bus_handle_t bus_handler;

	i2c_master_bus_config_t mbc = {
			.i2c_port = -1,
			.sda_io_num = GPIO_NUM_21,
			.scl_io_num = GPIO_NUM_22,
			.clk_source = I2C_CLK_SRC_DEFAULT,
			.glitch_ignore_cnt = 7,
			.flags.enable_internal_pullup = true
	};

	ESP_ERROR_CHECK( i2c_new_master_bus(&mbc, &bus_handler) );

	i2c_master_dev_handle_t tsc;
	i2c_add_dev(bus_handler, &tsc, TSC_I2C_ADDR);

	ESP_ERROR_CHECK( i2c_master_bus_reset(bus_handler) );
	
	ESP_ERROR_CHECK( i2c_master_probe(bus_handler, TSC_I2C_ADDR, -1) );
	
	i2csend(&tsc, 0b10001111); // select GAIN
	i2csend_and_check(&tsc, 0x00); // 1X
	
	i2csend(&tsc, 0b10000000); // select EN
	i2csend_and_check(&tsc, 0b00001011); // PON + WEN + AEN

	
	ESP_ERROR_CHECK( esp_netif_init() );
	ESP_ERROR_CHECK( esp_event_loop_create_default() );
	esp_netif_create_default_wifi_ap();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );

    wifi_config_t ap_config = {
		.ap = {
			.ssid = "superonline",
            .password = "sifresifre",
			.ssid_len = 11,
			.channel = 1,
			.authmode = WIFI_AUTH_WPA2_WPA3_PSK,
			.max_connection = 1,
			.pmf_cfg = {.capable = 1}
		}
	};

    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_AP, &ap_config) );
	
	//sensor_data_t* sensor_data = malloc(sizeof(sensor_data_t));
	//ringbuf_t* ring = malloc(sizeof(ringbuf_t));
	ringbuf_t ring_buf, ring_data;
	memset(&ring_buf, 0, sizeof(ringbuf_t));
	memset(&ring_data, 0, sizeof(ringbuf_t));
	ring_buf.buf = malloc(sizeof(sensor_t) * RING_BUF_SIZE);
	ring_data.buf = malloc(sizeof(data_avg_t) * RING_DATA_SIZE);

	if(ring_data.buf == 0 || ring_buf.buf == 0)
	{
		printf("malloc failed\n");
		vTaskDelay(500 / portTICK_PERIOD_MS);
		abort();
	}
	memset(ring_buf.buf, 0, sizeof(sensor_t) * RING_BUF_SIZE);
	memset(ring_data.buf, 0, sizeof(data_avg_t) * RING_DATA_SIZE);

	pthread_mutex_t mutex_buf, mutex_data;
	pthread_mutex_init(&mutex_buf, NULL);
	pthread_mutex_init(&mutex_data, NULL);
	wifi_sender_arg_t arg = {.ring = &ring_buf, .mutex = &mutex_buf};
	ESP_ERROR_CHECK( esp_event_handler_instance_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, sta_ip_assigned_handler, &arg, NULL) );
	ESP_ERROR_CHECK( esp_wifi_start() );


	uint16_t sec = 0, rgbc[4];
	uint8_t start;

	for(; ;)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		sec++;

		start = 0b10110100;
		for(uint8_t i = 0; i < 4; i++)
		{
			i2csend(&tsc, start++);
			uint8_t low = i2cget(&tsc);
			i2csend(&tsc, start++);
			uint16_t high = i2cget(&tsc);
			high <<= 8; high += low;
			rgbc[i] = high;
		}
		printf("(main) [%lu] r: %u g: %u b: %u c: %u\n", ring_buf.front, rgbc[1], rgbc[2], rgbc[3], rgbc[0]);

		((sensor_t*)ring_buf.buf)[ring_buf.front].r = rgbc[1];
		((sensor_t*)ring_buf.buf)[ring_buf.front].g = rgbc[2];
		((sensor_t*)ring_buf.buf)[ring_buf.front].b = rgbc[3];
		((sensor_t*)ring_buf.buf)[ring_buf.front].c = rgbc[0];
		
		if(sec % 30 == 0)
		{
			sec = 0;
			sensor_sum_t sum;
			memset(&sum, 0, sizeof(sensor_sum_t));
			uint32_t cur = ring_buf.front;

			for(uint16_t i = 0; i < RING_BUF_SIZE; i++)
			{
				for(uint16_t j = 0; j < SENSOR_COUNT; j++)
				{
					switch(i)
					{
						case 30:
							*(((data_avg_t*)ring_data.buf)[ring_data.front].sec_30.all+j) = (*(sum.all+j) / 30);
							break;
						case 60:
							*(((data_avg_t*)ring_data.buf)[ring_data.front].min_1.all+j)  = (*(sum.all+j) / 60);
							break;
						case 180:
							*(((data_avg_t*)ring_data.buf)[ring_data.front].min_3.all+j)  = (*(sum.all+j) / 180);
							break;
						case RING_BUF_SIZE-1:
							*(((data_avg_t*)ring_data.buf)[ring_data.front].min_9.all+j)  = (*(sum.all+j) / (RING_BUF_SIZE-1));
							break;
						default:
							goto out_of_for;
					}
				}

out_of_for:	
				sum.r += ((sensor_t*)ring_buf.buf)[cur].r;
				sum.g += ((sensor_t*)ring_buf.buf)[cur].g;
				sum.b += ((sensor_t*)ring_buf.buf)[cur].b;
				sum.c += ((sensor_t*)ring_buf.buf)[cur].c;
				if(cur == 0)
					cur = RING_BUF_SIZE-1;
				else
					cur--;
			}
		
			//ring_increment(&ring_data, &mutex_data, RING_DATA_SIZE);
		
			//test

			printf("////////\nsec_30:\n");
			for(uint8_t k = 0; k < 4; k++)
				printf("%u ",*(((data_avg_t*)ring_data.buf)[ring_data.front].sec_30.all+k));

			printf("\nmin_1:\n");
			for(uint8_t k = 0; k < 4; k++)
				printf("%u ",*(((data_avg_t*)ring_data.buf)[ring_data.front].min_1.all+k));

			printf("\nmin_3:\n");
			for(uint8_t k = 0; k < 4; k++)
				printf("%u ",*(((data_avg_t*)ring_data.buf)[ring_data.front].min_3.all+k));

			printf("\nmin_9:\n");
			for(uint8_t k = 0; k < 4; k++)
				printf("%u ",*(((data_avg_t*)ring_data.buf)[ring_data.front].min_9.all+k));
			printf("\n////////\n");

			//
			ring_increment(&ring_data, &mutex_data, RING_DATA_SIZE);
		
		
		}

		ring_increment(&ring_buf, &mutex_buf, RING_BUF_SIZE);

	}
	

}

static void i2c_add_dev(i2c_master_bus_handle_t bus_handler, i2c_master_dev_handle_t *dev_handler, const uint8_t addr)
{
	i2c_device_config_t devcfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = addr,
    .scl_speed_hz = 100000,
	};
	
	ESP_ERROR_CHECK( i2c_master_bus_add_device(bus_handler, &devcfg, dev_handler) );
}

static void i2csend(i2c_master_dev_handle_t* tsc, const uint8_t mes)
{
	uint8_t m = mes;
	ESP_ERROR_CHECK( i2c_master_transmit(*tsc, &m, 1, -1) );
}

static uint8_t i2cget(i2c_master_dev_handle_t* tsc)
{
	uint8_t m;
	ESP_ERROR_CHECK( i2c_master_receive(*tsc, &m, 1, -1) );
	return m;
}

static void i2csend_and_check(i2c_master_dev_handle_t* tsc, const uint8_t mes)
{
	i2csend(tsc, mes);
	if(i2cget(tsc) != mes)
	{
		printf("error writing %#x\n", mes);
		vTaskDelay(100000 / portTICK_PERIOD_MS);
		abort();
	}
}

static void ring_increment(ringbuf_t* ring, pthread_mutex_t* mutex, const uint32_t size)
{
	ring->front = (ring->front + 1) % size;
	pthread_mutex_lock(mutex);
	if(ring->full)
		ring->back = ring->front;
	else if(ring->front == ring->back)
		ring->full = 1;
	pthread_mutex_unlock(mutex);

}


static void sta_ip_assigned_handler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	ringbuf_t* ring = ((wifi_sender_arg_t*)arg)->ring;
	pthread_mutex_t* mutex = ((wifi_sender_arg_t*)arg)->mutex;
	ip_event_ap_staipassigned_t* info = data;

	char strbuf[17];
	esp_ip4addr_ntoa(&(info->ip), strbuf, 16);
	printf("connected ip: %s\n", strbuf);

	int32_t sock, sock_fd; uint8_t try = 0;
	do
	{
		sock = socket(AF_INET,SOCK_STREAM, 0);
		if(sock == -1)
		{
			printf("err1 ");
			goto wait;
		}
	} while (sock == -1);
	
	uint8_t opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

	struct sockaddr_in local_addr = {.sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(31000)};

	try = 0;
	while( bind(sock, &local_addr, sizeof(local_addr)) )
		{
			printf("err2 ");
			goto wait;
		}
	
	try = 0;
	while( listen(sock, 1) )
		{
			printf("err3 ");
			goto wait;
		}

	printf("waiting for connection...\n");
	struct sockaddr_in peer_addr;
	socklen_t peer_addr_len = sizeof(peer_addr);
	do
	{
		sock_fd = accept(sock, &peer_addr, &peer_addr_len);
		if(sock_fd == -1)
			goto wait;
	} while (sock_fd == -1);

	char sendbuf[100] = {0};
	pthread_mutex_lock(mutex);
	while(ring->back != ring->front || ring->full)
	{
		pthread_mutex_unlock(mutex);
		uint8_t send_len = 0;
		send_len += sprintf(sendbuf, "(wifi handler) [%lu] ", ring->back);
		for(uint8_t i = 0; i < SENSOR_COUNT; i++)
			send_len += sprintf(sendbuf+send_len, "%u%s", *(((sensor_t*)ring->buf)[ring->back].all+i), ((i < (SENSOR_COUNT - 1))? ", " : "\n") );

		int16_t send_len_ret = send(sock_fd, sendbuf, send_len, MSG_MORE);
		assert(send_len_ret == send_len);
		pthread_mutex_lock(mutex);
		ring->back = (ring->back + 1) % RING_BUF_SIZE;
		if(ring->full) ring->full = 0;
	}
	pthread_mutex_unlock(mutex);
	char dot = '.';
	send(sock_fd, &dot, 1, 0);
	close(sock_fd);
	close(sock);
	return;


wait:
	if(try++ == 3)
				{
					printf("aborting\n");
					return;
				}
				
				printf("socket error\n");
				vTaskDelay(1000 / portTICK_PERIOD_MS);

}

