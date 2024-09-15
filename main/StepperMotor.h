#pragma once

#include "driver/ledc.h"
#include "esp_err.h"

class StepperMotor {
private:
    ledc_channel_config_t ledc_channel;
    ledc_timer_config_t ledc_timer;
    int gpio_num;
    ledc_mode_t speed_mode;
    ledc_channel_t channel_num;
    ledc_timer_t timer_num;

public:
    StepperMotor(int gpio_num, ledc_timer_t timer_num, ledc_channel_t channel_num, ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE, int frequency = 1000) {
        this->gpio_num = gpio_num;
        this->speed_mode = speed_mode;
        this->channel_num = channel_num;
        this->timer_num = timer_num;

        // Timer configuration
        ledc_timer = {};
        ledc_timer.speed_mode = speed_mode;
        ledc_timer.timer_num = timer_num;
        ledc_timer.duty_resolution = LEDC_TIMER_14_BIT; // Increased duty resolution
        ledc_timer.freq_hz = frequency;
        ledc_timer.clk_cfg = LEDC_AUTO_CLK;

        // Channel configuration
        ledc_channel = {};
        ledc_channel.speed_mode = speed_mode;
        ledc_channel.channel = channel_num;
        ledc_channel.timer_sel = timer_num;
        ledc_channel.intr_type = LEDC_INTR_DISABLE;
        ledc_channel.gpio_num = gpio_num;

        // Set duty to 50% for square wave
        int max_duty = (1 << ledc_timer.duty_resolution) - 1;
        ledc_channel.duty = max_duty / 2;  // 50% duty cycle
        ledc_channel.hpoint = 0;

        // Initialize the LEDC peripheral with these settings
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }

    void setFrequency(int frequency) {
        ledc_timer.freq_hz = frequency;
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    }

    void start() {
        ESP_ERROR_CHECK(ledc_timer_resume(speed_mode, timer_num));
    }

    void stop() {
        ESP_ERROR_CHECK(ledc_timer_pause(speed_mode, timer_num));
    }
};
