#include "SDL_audio.h"

SDL_AudioSpec as;
//unsigned char *sdl_buffer;
unsigned char *sdl_buffer; //[SAMPLECOUNT * SAMPLESIZE * 2];
void *user_data;
bool paused = true;
bool locked = false;
xSemaphoreHandle xSemaphoreAudio = NULL;

IRAM_ATTR void updateTask(void *arg)
{
  size_t bytesWritten;
  while(1)
  {
	  if(!paused && /*xSemaphoreAudio != NULL*/ !locked ){
		  //xSemaphoreTake( xSemaphoreAudio, portMAX_DELAY );
		  memset(sdl_buffer, 0, SAMPLECOUNT*SAMPLESIZE*2);
		  (*as.callback)(NULL, sdl_buffer, SAMPLECOUNT*SAMPLESIZE );
		  ESP_ERROR_CHECK(i2s_write(I2S_NUM_0, sdl_buffer, SAMPLECOUNT*SAMPLESIZE*2, &bytesWritten, 50 / portTICK_PERIOD_MS));
		  //xSemaphoreGive( xSemaphoreAudio );
	  } else
		  vTaskDelay( 5 );
  }
}

void SDL_AudioInit()
{
	sdl_buffer = heap_caps_malloc(SAMPLECOUNT * SAMPLESIZE * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);

	static const i2s_config_t i2s_config = {
	.mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
	.sample_rate = SAMPLERATE,
	.bits_per_sample = SAMPLESIZE*8, /* the DAC module will only take the 8bits from MSB */
	.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
	.communication_format = I2S_COMM_FORMAT_I2S_MSB,
	.dma_buf_count = 4,
	.dma_buf_len = 256,
	.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,                                //Interrupt level 1
    .use_apll = 0
	};
	static const int i2s_num = I2S_NUM_0; // i2s port number

	ESP_ERROR_CHECK(i2s_driver_install(i2s_num, &i2s_config, 0, NULL));   //install and start i2s driver

	ESP_ERROR_CHECK(i2s_set_pin(i2s_num, NULL));
	//ESP_ERROR_CHECK(i2s_set_clk(i2s_num, SAMPLERATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO));
	ESP_ERROR_CHECK(i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN));	
}

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
	SDL_AudioInit();
	memset(obtained, 0, sizeof(SDL_AudioSpec)); 
	obtained->freq = SAMPLERATE;
	obtained->format = 16;
	obtained->channels = 1;
	obtained->samples = SAMPLECOUNT;
	obtained->callback = desired->callback;
	memcpy(&as,obtained,sizeof(SDL_AudioSpec));  

	//xSemaphoreAudio = xSemaphoreCreateBinary();
	xTaskCreatePinnedToCore(&updateTask, "updateTask", 10000, NULL, 2, NULL, 1);
	printf("audio task started\n");
	return 0;
}

void SDL_PauseAudio(int pause_on)
{
	paused = pause_on;
}

void SDL_CloseAudio(void)
{
	  i2s_driver_uninstall(I2S_NUM_0); //stop & destroy i2s driver
	  free(sdl_buffer);
}

int SDL_BuildAudioCVT(SDL_AudioCVT *cvt, Uint16 src_format, Uint8 src_channels, int src_rate, Uint16 dst_format, Uint8 dst_channels, int dst_rate)
{
	cvt->len_mult = 1;
	return 0;
}

IRAM_ATTR int SDL_ConvertAudio(SDL_AudioCVT *cvt)
{

	Sint16 *sbuf = cvt->buf;
	Uint16 *ubuf = cvt->buf;

	int32_t dac0;
	int32_t dac1;

	for(int i = cvt->len-2; i >= 0; i-=2)
	{
		Sint16 range = sbuf[i/2] >> 8; 

		// Convert to differential output
		if (range > 127)
		{
			dac1 = (range - 127);
			dac0 = 127;
		}
		else if (range < -127)
		{
			dac1  = (range + 127);
			dac0 = -127;
		}
		else
		{
			dac1 = 0;
			dac0 = range;
		}

		dac0 += 0x80;
		dac1 = 0x80 - dac1;

		dac0 <<= 8;
		dac1 <<= 8;

		ubuf[i] = (int16_t)dac1;
        ubuf[i + 1] = (int16_t)dac0;
	}

	return 0;
}

void SDL_LockAudio(void)
{
	locked = true;
	//if( xSemaphoreAudio != NULL )
	//	xSemaphoreTake( xSemaphoreAudio, 100 );
}

void SDL_UnlockAudio(void)
{
    locked = false;
	//if( xSemaphoreAudio != NULL )
	//	 xSemaphoreGive( xSemaphoreAudio );
}

