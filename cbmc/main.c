#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "dht11/dht11.c"

#define SENSOR_COUNT 4
typedef struct
{
	int8_t all[0];
	int8_t surf_temp;
	int8_t surf_humid;
	int8_t amb_temp;
	int8_t amb_humid;
} sensor_t;

typedef struct
{
	int32_t all[0];
	int32_t surf_temp;
	int32_t surf_humid;
	int32_t amb_temp;
	int32_t amb_humid;
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

#define RING_BUF_SIZE 8
#define RING_DATA_SIZE 14
typedef struct
{
	uint32_t front;
	uint32_t back;
	uint32_t full;
	uint32_t empty;
	void* 	 buf;
} ringbuf_t;


static void ring_increment(ringbuf_t*, /*pthread_mutex_t*,*/ const uint32_t) __attribute__((noinline));


void app_main(void)
{
	ringbuf_t ring_buf, ring_data;
	__CPROVER_havoc_slice( &ring_buf, sizeof(ringbuf_t) );
	__CPROVER_havoc_slice( &ring_data, sizeof(ringbuf_t) );

	__CPROVER_assume( ring_data.full < 2 && ring_data.empty < 2 && ring_buf.full < 2 && ring_buf.empty < 2 );
	__CPROVER_assume( !(ring_buf.full == 1 && ring_buf.empty == 1) );
	__CPROVER_assume( !(ring_data.full == 1 && ring_data.empty == 1) );
	__CPROVER_assume( ring_data.front < RING_DATA_SIZE );
	__CPROVER_assume( ring_buf.front < RING_BUF_SIZE );
	__CPROVER_assume( ring_data.back < RING_DATA_SIZE );
	__CPROVER_assume( ring_buf.back < RING_BUF_SIZE );
	ring_buf.buf = malloc(sizeof(sensor_t) * RING_BUF_SIZE);
	if(ring_buf.buf == 0) return;
	ring_data.buf = malloc(sizeof(data_avg_t) * RING_DATA_SIZE);
	if(ring_data.buf == 0)
	{
		free(ring_buf.buf);
		return;
	}


	//uint64_t nondet_x1;
	__CPROVER_assert( __CPROVER_rw_ok(ring_buf.buf, sizeof(sensor_t)*RING_BUF_SIZE), "ring_buf.buf rw" );
	__CPROVER_assert( __CPROVER_rw_ok(ring_data.buf, sizeof(data_avg_t)*RING_DATA_SIZE), "ring_data.buf rw" );
	

	uint64_t nondet_x2 = 2;

	struct dht11_reading dht_reading_arr[nondet_x2];
	uint8_t dht_pin_arr[nondet_x2];

	__CPROVER_havoc_slice(dht_pin_arr, nondet_x2);

	DHT11_init(dht_pin_arr, nondet_x2);

	uint16_t nondet_sec;

	for(uint8_t one = 0; one < 16; one++)
	{
		if(nondet_sec == UINT16_MAX) nondet_sec = 0;
		else nondet_sec++;

		uint64_t nondet_y1;
		uint64_t nondet_y2;

		__CPROVER_assume( nondet_y1 != nondet_y2 );
		__CPROVER_assume( (nondet_y1 < nondet_x2) && (nondet_y2 < nondet_x2) );

		dht_reading_arr[nondet_y1] = DHT11_read(dht_pin_arr[nondet_y1]);
		dht_reading_arr[nondet_y2] = DHT11_read(dht_pin_arr[nondet_y2]);

		uint64_t nondet_y3;

		__CPROVER_assume( (nondet_y3 == nondet_y1) || (nondet_y3 == nondet_y2) );

		if(
				dht_reading_arr[nondet_y3].temperature < -40 || 
				dht_reading_arr[nondet_y3].temperature > 255 ||
				dht_reading_arr[nondet_y3].humidity < 0 || 
				dht_reading_arr[nondet_y3].humidity > 100)
		{
			dht_reading_arr[nondet_y3] = DHT11_read(dht_pin_arr[nondet_y3]);
		}

		((sensor_t*)ring_buf.buf)[ring_buf.front].surf_temp = dht_reading_arr[nondet_y1].temperature;
		((sensor_t*)ring_buf.buf)[ring_buf.front].surf_humid = dht_reading_arr[nondet_y1].humidity;
		((sensor_t*)ring_buf.buf)[ring_buf.front].amb_temp = dht_reading_arr[nondet_y2].temperature;
		((sensor_t*)ring_buf.buf)[ring_buf.front].amb_humid = dht_reading_arr[nondet_y2].humidity;
		

		if(nondet_sec % 30 == 0)
		{
			nondet_sec = 0;
			sensor_sum_t sum;
			memset(&sum, 0, sizeof(sensor_sum_t));
			uint32_t cur = ring_buf.front;

			uint16_t nondet_sens_count;
			uint16_t nondet_sens_count2;
			uint16_t nondet_dur;
			uint64_t nondet_cur;
			__CPROVER_assume( (nondet_cur <= (RING_BUF_SIZE-1)) && (0 <= nondet_cur) );
			__CPROVER_assume( (nondet_dur <= 4) && (0 <= nondet_dur) );
			__CPROVER_assume( nondet_sens_count < SENSOR_COUNT );
			__CPROVER_assume( nondet_sens_count2 < SENSOR_COUNT );
			__CPROVER_assert( __CPROVER_w_ok( ((data_avg_t*)ring_data.buf)[ring_data.front].all[nondet_dur].all+nondet_sens_count, 1), "ring_data w" ); //uint8_t = 1
			__CPROVER_assert( __CPROVER_r_ok( ((sensor_t*)ring_buf.buf)[nondet_cur].all+nondet_sens_count2, 1 ), "ring_buf r" );
			

			uint8_t nondet_buttonpressed, nondet_icepresent;
			if(nondet_buttonpressed)
			{
				nondet_buttonpressed = 0;
				if(nondet_icepresent)
					nondet_icepresent = 0;
				else
				{
					if((int64_t)ring_data.front-(int64_t)ring_data.back < 6)
						printf("no backwards\n");
					else
					{
						uint16_t tmpcur = ring_data.front;
						
						((data_avg_t*)ring_data.buf)[tmpcur].ice = 1;
						if(tmpcur-- == 0) tmpcur = RING_DATA_SIZE - 1;
						((data_avg_t*)ring_data.buf)[tmpcur].ice = 1;
						if(tmpcur-- == 0) tmpcur = RING_DATA_SIZE - 1;
						((data_avg_t*)ring_data.buf)[tmpcur].ice = 1;
						if(tmpcur-- == 0) tmpcur = RING_DATA_SIZE - 1;
						((data_avg_t*)ring_data.buf)[tmpcur].ice = 1;
						if(tmpcur-- == 0) tmpcur = RING_DATA_SIZE - 1;
						((data_avg_t*)ring_data.buf)[tmpcur].ice = 1;
						if(tmpcur-- == 0) tmpcur = RING_DATA_SIZE - 1;
						((data_avg_t*)ring_data.buf)[tmpcur].ice = 1;
						if(tmpcur-- == 0) tmpcur = RING_DATA_SIZE - 1;
						/*for(uint8_t i = 0; i < 6; i++)
						{
							((data_avg_t*)ring_data.buf)[tmpcur].ice = 1;
							if(tmpcur-- == 0) tmpcur = RING_DATA_SIZE - 1;
						}*/
					}
					nondet_icepresent = 1;
				}
			}

			if(nondet_icepresent)
				((data_avg_t*)ring_data.buf)[ring_data.front].ice = 1;
			else
				((data_avg_t*)ring_data.buf)[ring_data.front].ice = 0;


			uint8_t nondet_z1;

			

			__CPROVER_assume( (5 <= ring_data.front && (nondet_z1 <= ring_data.front) && ((ring_data.front-5) <= nondet_z1)) ||
								ring_data.front < 5 && 
									( (0 <= nondet_z1 && nondet_z1 <= ring_data.front)  ||
									 	( ( (RING_DATA_SIZE-(5-ring_data.front))  <= nondet_z1) || (nondet_z1 <= (RING_DATA_SIZE-1)) ) )
							);

			__CPROVER_assert(  (!nondet_buttonpressed) || 
							   (nondet_buttonpressed &&  ((data_avg_t*)ring_data.buf)[nondet_z1].ice == 1     )  
							, "ice");
			

			ring_increment(&ring_data, RING_DATA_SIZE);
		
		}

		ring_increment(&ring_buf, RING_BUF_SIZE);

	}
	
	
	free(ring_buf.buf);
	free(ring_data.buf);

}


static void ring_increment(ringbuf_t* ring, /*pthread_mutex_t* mutex,*/ const uint32_t size)
{
	ring->front = (ring->front + 1) % size;
	if(ring->full)
		ring->back = ring->front;
	else if(ring->front == ring->back)
		ring->full = 1;

}

