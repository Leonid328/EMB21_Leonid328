# hw_2.5: Non-blocking buzzer player PWM

LEDC buzzer on GPIO5. Superloop uses a 50 ms tick from esp_timer_get_time. No FreeRTOS task and no delay in the player. Melody: Twinkle Twinkle (C C G G A A G ...).
