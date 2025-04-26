// station.h
#pragma once

#include <string>
#include <vector>

struct StationId {
    std::string value;

    // Implicit conversion constructor
    StationId(const std::string& v)
        : value(v)
    {
    }
};

struct StationName {
    std::string value;

    // Implicit conversion constructor
    StationName(const std::string& v)
        : value(v)
    {
    }
};

class Station {
public:
    Station(const StationId& id, const StationName& name, int capacity);

    void setRentalRate(int timeSlot, double rate);
    void setReturnRate(int timeSlot, double rate);

    std::string getId() const;
    std::string getName() const;
    int getCapacity() const;
    double getRentalRate(int timeSlot) const;
    double getReturnRate(int timeSlot) const;

    static const int TIME_SLOTS = 48; // 48 30-minute intervals in a day

private:
    std::string id;
    std::string name;
    int capacity;
    std::vector<double> rentalRates; // Bikes per minute
    std::vector<double> returnRates; // Bikes per minute
};
