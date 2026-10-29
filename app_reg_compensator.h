#pragma once

#include <stdbool.h>
#include <stdint.h>


void compensate_heating(float temperature, bool relay_on, uint32_t now_ms, float *compensated_temp,
                        float coefficient_s);
