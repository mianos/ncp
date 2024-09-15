#include "StepperMotor.h"
#include "esp_log.h"
#include <algorithm>

StepperMotor::StepperMotor(gpio_num_t stepPin, double stepsPerRevolution, double mlPerRevolution)
    : stepPin(stepPin),
      stepsPerRevolution(stepsPerRevolution),
      mlPerRevolution(mlPerRevolution),
      volumePerMinute(0),
      stepFrequency(0),
      running(false) {

    // Configure the RMT transmitter channel
    rmt_tx_channel_config_t tx_config = {};
    tx_config.gpio_num = stepPin;
    tx_config.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_config.resolution_hz = 1e6;  // 1 MHz resolution
    tx_config.mem_block_symbols = 64;
    tx_config.trans_queue_depth = 4;  // Adjust as needed

    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_config, &rmtChannel));

    // Create an RMT encoder (copy encoder for simple step pulses)
    rmt_copy_encoder_config_t encoder_config = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&encoder_config, &encoder));

    // Enable the RMT channel
    ESP_ERROR_CHECK(rmt_enable(rmtChannel));
}

StepperMotor::~StepperMotor() {
    stop();
    ESP_ERROR_CHECK(rmt_del_encoder(encoder));
    ESP_ERROR_CHECK(rmt_del_channel(rmtChannel));
}

void StepperMotor::setFrequency(double frequency) {
    stepFrequency = frequency;
    configureRMT(frequency);
}

void StepperMotor::setVolumePerMinute(double volume) {
    volumePerMinute = volume;
    double stepsPerMinute = (volumePerMinute / mlPerRevolution) * stepsPerRevolution;
    setFrequency(stepsPerMinute / 60.0);
}


void StepperMotor::start() {
    if (!running) {
        ESP_ERROR_CHECK(rmt_enable(rmtChannel));

        // Transmit the RMT symbols
        rmt_transmit_config_t transmit_config = {};
        transmit_config.loop_count = 0;  // 0 means infinite looping

        ESP_ERROR_CHECK(rmt_transmit(rmtChannel, encoder, &rmtItem, sizeof(rmtItem), &transmit_config));

        running = true;
    }
}


void StepperMotor::stop() {
    if (running) {
        // Disable the RMT channel to stop transmission
        ESP_ERROR_CHECK(rmt_disable(rmtChannel));
        running = false;
    }
}

void StepperMotor::configureRMT(double frequency) {
    uint32_t period_us = static_cast<uint32_t>(1e6 / frequency);

    const uint32_t min_pulse_width_us = 3;

    uint32_t high_duration = std::max(period_us / 2, min_pulse_width_us);
    uint32_t low_duration = std::max(period_us - high_duration, min_pulse_width_us);

    // Store the RMT item as a class member to keep it in scope
    rmtItem.level0 = 1;
    rmtItem.duration0 = high_duration;
    rmtItem.level1 = 0;
    rmtItem.duration1 = low_duration;
}

