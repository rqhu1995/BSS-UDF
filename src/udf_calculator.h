// udf_calculator.h
#pragma once

#include "station.h"
#include <Eigen/Dense>
#include <map>
#include <unsupported/Eigen/MatrixFunctions>
#include <vector>

class UDFCalculator {
public:
    UDFCalculator(double penaltyRenter = 1.0, double penaltyReturner = 1.0);

    double calculateUDF(const Station& station, int initialInventory, double discretizationLevel = 30.0);
    std::vector<double> calculateUDFForAllInventories(const Station& station, double discretizationLevel = 30.0);
    int findOptimalInventory(const std::vector<double>& udfValues);

private:
    // Cache for transition matrices to avoid redundant calculations
    std::map<std::tuple<double, double, int, double>, Eigen::MatrixXd> transitionMatrixCache;

    double penaltyRenter; // p - penalty for each potential renter who abandons
    double penaltyReturner; // h - penalty for each returner who abandons

    // Calculate transition probability matrix for a given time period
    Eigen::MatrixXd calculateTransitionMatrix(double rentalRate, double returnRate,
        int capacity, double deltaTime);

    // Approximation of e^(R*delta) using the identity e^R = lim (I + R/n)^n
    Eigen::MatrixXd matrixExponential(const Eigen::MatrixXd& R, int iterations = 100);
    // Add to udf_calculator.h
    std::vector<double> calculateMarginalUDF(const Station& station, double discretizationLevel = 30.0);
};
