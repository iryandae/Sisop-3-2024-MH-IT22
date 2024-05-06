#include <stdio.h>
#include <string.h>
#include "actions.h"

const char* check_gap(float gap) {
    if (gap < 3.5) {
        return "Gogogo";
    } else if (gap <= 10) {
        return "Push";
    } else {
        return "Stay out of trouble";
    }
}

const char* check_fuel(float fuel) {
    if (fuel > 80) {
        return "Push Push Push";
    } else if (fuel >= 50) {
        return "You can go";
    } else {
        return "Conserve Fuel";
    }
}

const char* check_tire(int tire) {
    if (tire > 80) {
        return "Go Push Go Push";
    } else if (tire > 50) {
        return "Good Tire Wear";
    } else if (tire > 30) {
        return "Conserve Your Tire";
    } else {
        return "Box Box Box";
    }
}

const char* check_tire_change(const char* tire_type) {
    if (strcmp(tire_type, "Soft") == 0) {
        return "Mediums Ready";
    } else if (strcmp(tire_type, "Medium") == 0) {
        return "Box for Softs";
    } else {
        return "Invalid tire type";
    }
}
