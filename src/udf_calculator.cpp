// udf_calculator.cpp
#include "udf_calculator.h"
#include <algorithm>

UDFCalculator::UDFCalculator(double penaltyRenter, double penaltyReturner)
    : penaltyRenter(penaltyRenter)
    , penaltyReturner(penaltyReturner)
{
}

double UDFCalculator::calculateUDF(const Station& station, int initialInventory, double discretizationLevel)
{
    if (initialInventory < 0 || initialInventory > station.getCapacity()) {
        return std::numeric_limits<double>::max(); // Invalid inventory
    }

    int capacity = station.getCapacity();
    int numStates = capacity + 1; // States from 0 to capacity

    // Number of discrete time steps
    int timeSteps = static_cast<int>((24.0 * 60.0) / discretizationLevel);
    double deltaTime = discretizationLevel;

    // Initialize transition probability matrix
    Eigen::MatrixXd transitionProb = Eigen::MatrixXd::Identity(numStates, numStates);

    // UDF accumulator
    double udf = 0.0;

    // For each time step
    for (int step = 0; step < timeSteps; step++) {
        // Determine which 30-minute interval we're in
        int timeSlot = (step * static_cast<int>(discretizationLevel)) / 30;
        timeSlot = timeSlot % Station::TIME_SLOTS;

        // Get rental and return rates for this time slot
        double rentalRate = station.getRentalRate(timeSlot);
        double returnRate = station.getReturnRate(timeSlot);

        // Calculate transition matrix for this period
        Eigen::MatrixXd P = calculateTransitionMatrix(rentalRate, returnRate, capacity, (timeSlot + 1) * deltaTime);

        // Update transition probability
        transitionProb = transitionProb * P;

        // Calculate contribution to UDF
        // The probability of being empty (state 0) multiplied by rental rate and penalty
        double emptyProb = transitionProb(initialInventory, 0);
        // The probability of being full (state capacity) multiplied by return rate and penalty
        double fullProb = transitionProb(initialInventory, capacity);

        // Add to UDF: πI0,0(t)μt p + πI0,C(t)λt h
        udf += (emptyProb * rentalRate * penaltyRenter + fullProb * returnRate * penaltyReturner) * deltaTime;
    }

    return udf;
}

std::vector<double> UDFCalculator::calculateUDFForAllInventories(const Station& station, double discretizationLevel)
{
    std::vector<double> results;
    int capacity = station.getCapacity();

    for (int i = 0; i <= capacity; i++) {
        double udf = calculateUDF(station, i, discretizationLevel);
        results.push_back(udf);
    }

    return results;
}

int UDFCalculator::findOptimalInventory(const std::vector<double>& udfValues)
{
    // Find the inventory level with the minimum UDF
    auto optimalInventory = static_cast<int>(std::min_element(udfValues.begin(), udfValues.end()) - udfValues.begin());
    return optimalInventory;
}

// In udf_calculator.cpp
Eigen::MatrixXd UDFCalculator::calculateTransitionMatrix(double rentalRate, double returnRate,
    int capacity, double deltaTime)
{
    // Check if we've already calculated this matrix
    auto quantize = [](double x) { return std::round(x * 100.0) / 100.0; }; // 2 decimals
    auto key = std::make_tuple(quantize(rentalRate), quantize(returnRate), capacity, quantize(deltaTime));
    auto it = transitionMatrixCache.find(key);
    if (it != transitionMatrixCache.end()) {
        return it->second;
    }

    int size = capacity + 1;

    // Create rate matrix R
    Eigen::MatrixXd R = Eigen::MatrixXd::Zero(size, size);

    // Fill the rate matrix
    for (int i = 0; i <= capacity; i++) {
        // Diagonal elements
        R(i, i) = -((i > 0 ? rentalRate : 0) + (i < capacity ? returnRate : 0));

        // Transitions due to rentals (i -> i-1)
        if (i > 0) {
            R(i, i - 1) = rentalRate;
        }

        // Transitions due to returns (i -> i+1)
        if (i < capacity) {
            R(i, i + 1) = returnRate;
        }
    }

    // Calculate e^(R*deltaTime)
    // Eigen::MatrixXd result = matrixExponential(R * deltaTime);
    Eigen::MatrixXd result = (R * deltaTime).exp();

    // Cache the result
    transitionMatrixCache[key] = result;

    return result;
}

Eigen::MatrixXd UDFCalculator::matrixExponential(const Eigen::MatrixXd& R, int iterations)
{
    int size = R.rows();
    Eigen::MatrixXd result = Eigen::MatrixXd::Identity(size, size);
    Eigen::MatrixXd term = Eigen::MatrixXd::Identity(size, size);

    // Calculate (I + R/n)^n
    for (int i = 1; i <= iterations; i++) {
        term = term * (Eigen::MatrixXd::Identity(size, size) + R / iterations);
    }

    return term;
}

// Implementation in udf_calculator.cpp
std::vector<double> UDFCalculator::calculateMarginalUDF(const Station& station, double discretizationLevel)
{
    std::vector<double> udfValues = calculateUDFForAllInventories(station, discretizationLevel);
    std::vector<double> marginalValues;

    // F'(I0) = F(I0) - F(I0-1)
    for (int i = 1; i <= station.getCapacity(); i++) {
        marginalValues.push_back(udfValues[i] - udfValues[i - 1]);
    }

    return marginalValues;
}
