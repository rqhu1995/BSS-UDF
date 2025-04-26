// station.cpp
#include "station.h"

Station::Station(const StationId& id, const StationName& name, int capacity)
    : id(id.value)
    , name(name.value)
    , capacity(capacity)
{
    rentalRates.resize(TIME_SLOTS, 0.0);
    returnRates.resize(TIME_SLOTS, 0.0);
}

void Station::setRentalRate(int timeSlot, double rate)
{
    if (timeSlot >= 0 && timeSlot < TIME_SLOTS) {
        rentalRates[timeSlot] = rate;
    }
}

void Station::setReturnRate(int timeSlot, double rate)
{
    if (timeSlot >= 0 && timeSlot < TIME_SLOTS) {
        returnRates[timeSlot] = rate;
    }
}

std::string Station::getId() const
{
    return id;
}

std::string Station::getName() const
{
    return name;
}

int Station::getCapacity() const
{
    return capacity;
}

double Station::getRentalRate(int timeSlot) const
{
    if (timeSlot >= 0 && timeSlot < TIME_SLOTS) {
        return rentalRates[timeSlot];
    }
    return 0.0;
}

double Station::getReturnRate(int timeSlot) const
{
    if (timeSlot >= 0 && timeSlot < TIME_SLOTS) {
        return returnRates[timeSlot];
    }
    return 0.0;
}
