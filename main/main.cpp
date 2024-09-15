#include <stdio.h>

#include "esp_log.h"

#include "WifiManager.h"
#include "SettingsManager.h"
#include "WebServer.h"
#include "StepperMotor.h"

static const char *TAG = "npc";

static SemaphoreHandle_t wifiSemaphore;

static void localEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
	    xSemaphoreGive(wifiSemaphore);
	}
}


extern "C" void app_main() {
	NvsStorageManager nv;
	SettingsManager settings(nv);

	wifiSemaphore = xSemaphoreCreateBinary();
	WiFiManager wifiManager(nv, localEventHandler, nullptr);
//	wifiManager.clear();
    if (xSemaphoreTake(wifiSemaphore, portMAX_DELAY) ) {
		ESP_LOGI(TAG, "Main task continues after WiFi connection.");

        static WebServer webServer;
        if (webServer.start() == ESP_OK) {
            ESP_LOGI(TAG, "Web server started successfully.");
        } else {
            ESP_LOGE(TAG, "Failed to start web server.");
        }

    ledc_timer_t timer_num = LEDC_TIMER_0;
    ledc_channel_t channel_num = LEDC_CHANNEL_0;

    // Instantiate the StepperMotor class with 10 Hz frequency
    StepperMotor stepperMotor(
        GPIO_NUM_42,           // GPIO number connected to STEP pin
        timer_num,          // Timer number
        channel_num,        // Channel number
        LEDC_LOW_SPEED_MODE,// Speed mode
        10                  // Set frequency to 10 Hz
    );

	    stepperMotor.start();

		while (true) {
			vTaskDelay(pdMS_TO_TICKS(100)); 
		
		}
	}

}
