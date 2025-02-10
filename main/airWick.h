#pragma once

#include <inttypes.h>
#include <sdkconfig.h>
#include <stdint.h>

extern int32_t spray_counter;


void airWickSetup();

void airWickMotorUP();

void airWickMotorDown();

void airWickSpray();