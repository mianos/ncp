#pragma once
#include "driver/rmt_tx.h"
#include "driver/gpio.h"

class StepperMotor {
public:
    StepperMotor(gpio_num_t stepPin, double stepsPerRevolution, double mlPerRevolution);
    ~StepperMotor();

    void setFrequency(double frequency);
    void start();
    void stop();
    void setVolumePerMinute(double volume);

private:
    void configureRMT(double frequency);

    gpio_num_t stepPin;
    double stepsPerRevolution;
    double mlPerRevolution;
    double volumePerMinute;
    double stepFrequency;
    bool running;

    rmt_channel_handle_t rmtChannel;
    rmt_encoder_handle_t encoder;

    // Store the RMT item as a member to keep it in scope
    rmt_symbol_word_t rmtItem;
};

