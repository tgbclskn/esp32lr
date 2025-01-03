#include <stdio.h>
#include "esp_wifi.h"
#include "driver/i2c_master.h"
#include "pthread.h"
#include "string.h"
#include "lwip/sockets.h"
#include "esp_random.h"
#include "dht11/dht11.c"
#include "Logistic-Regression/regression/logistic.c"
#include "Logistic-Regression/numerical/matrix.c"
#include "Logistic-Regression/preprocessing/scaler.c"

#define TSC_I2C_ADDR 0x29

#define SENSOR_COUNT 4
typedef struct
{
	uint8_t all[0];
	//uint16_t r;
	//uint16_t g;
	//uint16_t b;
	//uint16_t c;
	uint8_t surf_temp;
	uint8_t surf_humid;
	uint8_t amb_temp;
	uint8_t amb_humid;
} sensor_t;

typedef struct
{
	uint32_t all[0];
	//uint32_t r;
	//uint32_t g;
	//uint32_t b;
	//uint32_t c;
	uint32_t surf_temp;
	uint32_t surf_humid;
	uint32_t amb_temp;
	uint32_t amb_humid;
} sensor_sum_t;

#define DUR_COUNT 4
typedef struct
{
	sensor_t all[0];
	sensor_t sec_30;
	sensor_t min_1;
	sensor_t min_3;
	sensor_t min_9;
	uint32_t ice;
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

volatile uint8_t DRAM_ATTR buttonpressed = 0;


/*static void i2c_add_dev(i2c_master_bus_handle_t, i2c_master_dev_handle_t*, const uint8_t) __attribute__((noinline));
static void i2csend_and_check(i2c_master_dev_handle_t*, const uint8_t) __attribute__((noinline));
static void i2csend(i2c_master_dev_handle_t*, const uint8_t) __attribute__((noinline));
static uint8_t i2cget(i2c_master_dev_handle_t*) __attribute__((noinline));*/
static void sta_ip_assigned_handler(void*, esp_event_base_t, int32_t, void*);
static void ring_increment(ringbuf_t*, pthread_mutex_t*, const uint32_t) __attribute__((noinline));
static uint8_t retry(uint8_t*, char*) __attribute__((noinline));
static void f_buttonpressed(void*);

#define BUTTON_PIN GPIO_NUM_18

void app_main(void)
{
	gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
	TaskHandle_t f_buttonpressed_taskhandle;
	xTaskCreate(f_buttonpressed, "buttontask", configMINIMAL_STACK_SIZE * 2, NULL, uxTaskPriorityGet(NULL), &f_buttonpressed_taskhandle);


    /*i2c_master_bus_handle_t bus_handler;

	i2c_master_bus_config_t mbc = {
			.i2c_port = -1,
			.sda_io_num = GPIO_NUM_21,
			.scl_io_num = GPIO_NUM_22,
			.clk_source = I2C_CLK_SRC_DEFAULT,
			.glitch_ignore_cnt = 7,
			.flags.enable_internal_pullup = true
	};*/

	//ESP_ERROR_CHECK( i2c_new_master_bus(&mbc, &bus_handler) );

	//i2c_master_dev_handle_t tsc;
	//i2c_add_dev(bus_handler, &tsc, TSC_I2C_ADDR);

	//ESP_ERROR_CHECK( i2c_master_bus_reset(bus_handler) );
	
	//ESP_ERROR_CHECK( i2c_master_probe(bus_handler, TSC_I2C_ADDR, -1) );
	
	//i2csend(&tsc, 0b10001111); // select GAIN
	//i2csend_and_check(&tsc, 0x00); // 1X
	
	//i2csend(&tsc, 0b10000000); // select EN
	//i2csend_and_check(&tsc, 0b00001011); // PON + WEN + AEN


	//wifi & alloc
	

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
	
	ringbuf_t ring_buf, ring_data;
	memset(&ring_buf, 0, sizeof(ringbuf_t));
	memset(&ring_data, 0, sizeof(ringbuf_t));
	heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
	ring_buf.buf = malloc(sizeof(sensor_t) * RING_BUF_SIZE);
	heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);

	ring_data.buf = malloc(sizeof(data_avg_t) * RING_DATA_SIZE);
	heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);


	if(ring_data.buf == 0 || ring_buf.buf == 0)
	{
		printf("malloc failed\n");
		vTaskDelay(UINT16_MAX / portTICK_PERIOD_MS);
		abort();
	}
	memset(ring_buf.buf, 0, sizeof(sensor_t) * RING_BUF_SIZE);
	memset(ring_data.buf, 0, sizeof(data_avg_t) * RING_DATA_SIZE);

	pthread_mutex_t mutex_buf, mutex_data;
	pthread_mutex_init(&mutex_buf, NULL);
	pthread_mutex_init(&mutex_data, NULL);
	wifi_sender_arg_t arg = {.ring = &ring_data, .mutex = &mutex_data};
	ESP_ERROR_CHECK( esp_event_handler_instance_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, sta_ip_assigned_handler, &arg, NULL) );
	ESP_ERROR_CHECK( esp_wifi_start() );


	//dht surf + amb

	
	struct dht11_reading dht11_surf, dht11_amb, dht11_surf_old, dht11_amb_old;
	#define DHT11_SURF_PIN 32
	#define DHT11_AMB_PIN 33
	#define DHT11_SURF 0
	#define DHT11_AMB 1
	DHT11_init((gpio_num_t[]){DHT11_SURF_PIN, DHT11_AMB_PIN}, 2);
	usleep(10*1000);

	double ss_mean[] = {30.804878, 44.795122};
	double ss_std[] = {2.022067, 23.898504};
	const StandardScaler ss = {.mean = ss_mean, .std = ss_std};
	Matrix W = zeros(2, 1);
	Matrix b = zeros(1, 1);
	
	
	W.data[0][0] = 0.56;
	W.data[1][0] = 1.27;
	b.data[0][0] = -2.39;

	const Model trained_model = {.ss = ss, .W = W, .b = b};
	Matrix inmatrix = zeros(1,2);

	/*while(1)
	{
		dht11_surf = DHT11_read(DHT11_SURF);
		dht11_amb = DHT11_read(DHT11_AMB);
		usleep(1000 * 2100);
		inmatrix.data[0][0] = (double)dht11_surf.temperature;
		inmatrix.data[0][1] = (double)dht11_surf.humidity;
		const Matrix surf_predict = predict(trained_model, inmatrix);
		inmatrix.data[0][0] = (double)dht11_amb.temperature;
		inmatrix.data[0][1] = (double)dht11_amb.humidity;
		const Matrix amb_predict = predict(trained_model, inmatrix);
		printf("surf temp:%u humid:%u predict:%lf\n",dht11_surf.temperature,dht11_surf.humidity, surf_predict.data[0][0]);
		printf("amb temp:%u humid:%u predict:%lf\n",dht11_amb.temperature,dht11_amb.humidity, amb_predict.data[0][0]);
		printf("pressed:%u\n",buttonpressed);
		dispose(surf_predict);
		dispose(amb_predict);
	}

	return;*/
	
	
	uint16_t sec = 0 /*, rgbc[4]*/;
	uint8_t start, icepresent = 0;

	for(; ;)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		sec++;

		//tsc
		/*start = 0b10110100;
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
		((sensor_t*)ring_buf.buf)[ring_buf.front].c = rgbc[0];*/
		
		
		dht11_surf = DHT11_read(DHT11_SURF);
		dht11_amb = DHT11_read(DHT11_AMB);
		while(dht11_surf.temperature < 0)
		{
			printf("dht11_surf val err\n");
			usleep(1000 * 1000);
			dht11_surf = DHT11_read(DHT11_SURF);
		}
		
		while(dht11_amb.temperature < 0)
		{
			printf("dht11_amb val err\n");
			usleep(1000 * 1000);
			dht11_amb = DHT11_read(DHT11_AMB);
		
		}

		printf("========\n%03u,%03u %03u,%03u ice:%u\n", dht11_surf.temperature, dht11_surf.humidity, dht11_amb.temperature, dht11_amb.humidity, icepresent);
		
		((sensor_t*)ring_buf.buf)[ring_buf.front].surf_temp = dht11_surf.temperature;
		((sensor_t*)ring_buf.buf)[ring_buf.front].surf_humid = dht11_surf.humidity;
		((sensor_t*)ring_buf.buf)[ring_buf.front].amb_temp = dht11_amb.temperature;
		((sensor_t*)ring_buf.buf)[ring_buf.front].amb_humid = dht11_amb.humidity;
		

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
				/*sum.r += ((sensor_t*)ring_buf.buf)[cur].r;
				sum.g += ((sensor_t*)ring_buf.buf)[cur].g;
				sum.b += ((sensor_t*)ring_buf.buf)[cur].b;
				sum.c += ((sensor_t*)ring_buf.buf)[cur].c;*/
				sum.surf_temp += ((sensor_t*)ring_buf.buf)[cur].surf_temp;
				sum.surf_humid += ((sensor_t*)ring_buf.buf)[cur].surf_humid;
				sum.amb_temp += ((sensor_t*)ring_buf.buf)[cur].amb_temp;
				sum.amb_humid += ((sensor_t*)ring_buf.buf)[cur].amb_humid;
				if(cur == 0)
					cur = RING_BUF_SIZE-1;
				else
					cur--;
			}

			//ice

			if(buttonpressed)
			{
				buttonpressed = 0;
				if(icepresent)
					icepresent = 0;
				else
				{
					if(ring_data.front-ring_data.back < 6)
						printf("no backwards\n");
					else
					{
						uint16_t tmpcur = ring_data.front;
						for(uint8_t i = 0; i < 6; i++)
						{
							((data_avg_t*)ring_data.buf)[tmpcur].ice = 1;
							if(tmpcur-- == 0) tmpcur = RING_DATA_SIZE - 1;
						}
					}
					icepresent = 1;
				}
			}

			if(icepresent)
				((data_avg_t*)ring_data.buf)[ring_data.front].ice = 1;
			else
				((data_avg_t*)ring_data.buf)[ring_data.front].ice = 0;

			/*uint8_t ice_r;
			esp_fill_random(&ice_r, 1);
			if(ice_r % 2 == 0)
				ice_r = 0;
			else
				ice_r = 1;

			((data_avg_t*)ring_data.buf)[ring_data.front].ice = ice_r;*/
		
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

			printf("\nice: %lu",((data_avg_t*)ring_data.buf)[ring_data.front].ice);
			printf("\n////////\n");

			//
			ring_increment(&ring_data, &mutex_data, RING_DATA_SIZE);
		
		
		}

		ring_increment(&ring_buf, &mutex_buf, RING_BUF_SIZE);

		inmatrix.data[0][0] = (double)dht11_surf.temperature;
		inmatrix.data[0][1] = (double)dht11_surf.humidity;
		const Matrix surf_predict = predict(trained_model, inmatrix);
		inmatrix.data[0][0] = (double)dht11_amb.temperature;
		inmatrix.data[0][1] = (double)dht11_amb.humidity;
		const Matrix amb_predict = predict(trained_model, inmatrix);
		printf("surf predict:%u amb predict:%u\n========\n",(uint8_t)surf_predict.data[0][0],(uint8_t)amb_predict.data[0][0]);
		dispose(surf_predict);
		dispose(amb_predict);

	}
	

}

/*static void i2c_add_dev(i2c_master_bus_handle_t bus_handler, i2c_master_dev_handle_t *dev_handler, const uint8_t addr)
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
}*/

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

	int32_t sock, sock_fd; uint8_t try = 0;
	char* txt;
	do
	{
		sock = socket(AF_INET,SOCK_STREAM, 0);
		if(sock == -1)
		{
			txt = "err1\0";
			if(retry(&try, txt))
				goto exit;
		}
	} while (sock == -1);
	
	int opts = fcntl(sock, F_GETFL, NULL);
	opts |= O_NONBLOCK;
	fcntl(sock, F_SETFL, opts);

	const uint32_t opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

	struct sockaddr_in local_addr = {.sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(31000)};

	try = 0;
	while( bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) )
		{
			txt = "err2\0";
			if(retry(&try, txt))
				goto exit_sock;
		}
	
	try = 0;
	while( listen(sock, 1) )
		{
			txt = "err3\0";
			if(retry(&try, txt))
				goto exit_sock;
		}

	struct sockaddr_in peer_addr;
	socklen_t peer_addr_len = sizeof(peer_addr);

	try = 0;
	do
	{
		sock_fd = accept(sock, (struct sockaddr*)&peer_addr, &peer_addr_len);
		if(sock_fd == -1)
		{
			int accept_ret = errno;
			if(accept_ret == EAGAIN)
				txt = "EAGAIN\0";
			else
				txt = "err4\0";
			if(retry(&try, txt))
				goto exit_sock;
		}
	} while (sock_fd == -1);
	printf("accepted\n");

	char sendbuf[(SENSOR_COUNT * DUR_COUNT * 7) + 10] = {0};
	pthread_mutex_lock(mutex);

	while(ring->back != ring->front || ring->full)
	{
		pthread_mutex_unlock(mutex);
		
		uint8_t send_len = 0;
//		send_len += sprintf(sendbuf, "(wifi handler) [%lu] \n", ring->back);
		for(uint8_t duration = 0; duration < DUR_COUNT; duration++)
			for(uint8_t sensor = 0; sensor < SENSOR_COUNT; sensor++)
				send_len += sprintf(sendbuf+send_len, "%u, ", *((*(((data_avg_t*)ring->buf)[ring->back].all+duration)).all+sensor) /*, ((sensor != (SENSOR_COUNT - 1) || duration != (DUR_COUNT - 1))? ", " : "\n")*/ );

		send_len += sprintf(sendbuf+send_len, "%lu\n", ((data_avg_t*)ring->buf)[ring->back].ice);

		int16_t send_len_ret = send(sock_fd, sendbuf, send_len, MSG_MORE);
		assert(send_len_ret == send_len);
		pthread_mutex_lock(mutex);
		ring->back = (ring->back + 1) % RING_BUF_SIZE;
		if(ring->full) ring->full = 0;
	}
	
	pthread_mutex_unlock(mutex);
	
	char* dot = "\n.";
	send(sock_fd, dot, 2, 0);
	
	close(sock_fd);

exit_sock:
	close(sock);

exit:
	ESP_ERROR_CHECK( esp_wifi_deauth_sta(0) );
	ESP_ERROR_CHECK( esp_wifi_stop() );
	vTaskDelay(1);
	ESP_ERROR_CHECK( esp_wifi_start() );
	return;
}

uint8_t retry(uint8_t* try, char* text)
{

	if((*try)++ == 10)
	{
		printf("aborting\n");
		return 1;
	}
		
	printf("%s\n",text);
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	return 0;
}

void f_buttonpressed(void* p)
{
	for(; ;)
	{
		if(buttonpressed == 0)
			if(gpio_get_level(BUTTON_PIN) == 0) buttonpressed = 1;
		usleep(1000 * 125);
	}
}