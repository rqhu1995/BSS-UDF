// main.cpp
#include "station.h"
#include "udf_calculator.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Helper function to split a string by delimiter
std::vector<std::string> split(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Parse the CSV file and create Station objects
std::vector<Station> parseStationsFromCSV(const std::string& filename)
{
    std::vector<Station> stations;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return stations;
    }

    std::string line;
    // Read header line
    std::getline(file, line);
    std::vector<std::string> headers = split(line, ',');

    // Map column indices
    std::map<int, int> rentalTimeSlots;
    std::map<int, int> returnTimeSlots;

    for (int i = 0; i < headers.size(); i++) {
        if (headers[i].find("rentalrate_") != std::string::npos) {
            // Extract the time slot from column name
            std::string timeStr = headers[i].substr(11);
            int hour = std::stoi(timeStr.substr(0, 2));
            int minute = std::stoi(timeStr.substr(3, 2));
            int timeSlot = (hour * 60 + minute) / 30;
            rentalTimeSlots[i] = timeSlot;
        } else if (headers[i].find("returnrate_") != std::string::npos) {
            // Extract the time slot from column name
            std::string timeStr = headers[i].substr(11);
            int hour = std::stoi(timeStr.substr(0, 2));
            int minute = std::stoi(timeStr.substr(3, 2));
            int timeSlot = (hour * 60 + minute) / 30;
            returnTimeSlots[i] = timeSlot;
        }
    }

    // Read data rows
    while (std::getline(file, line)) {
        std::vector<std::string> values = split(line, ',');

        if (values.size() != headers.size()) {
            std::cerr << "Warning: Row has incorrect number of fields. Skipping." << std::endl;
            continue;
        }

        const std::string& stationId = values[0];
        const std::string& stationName = values[1];
        int capacity = std::stoi(values[values.size() - 1]);

        Station station(StationId { stationId }, StationName { stationName }, capacity);

        // Set rental and return rates
        for (const auto& [colIndex, timeSlot] : rentalTimeSlots) {
            if (colIndex < values.size()) {
                double rate = std::stod(values[colIndex]);
                station.setRentalRate(timeSlot, rate);
            }
        }

        for (const auto& [colIndex, timeSlot] : returnTimeSlots) {
            if (colIndex < values.size()) {
                double rate = std::stod(values[colIndex]);
                station.setReturnRate(timeSlot, rate);
            }
        }

        stations.push_back(station);
    }

    file.close();
    return stations;
}

void writeCSVResults(const std::vector<Station>& stations,
    const std::vector<std::vector<double>>& udfValues,
    const std::vector<int>& optimalInventories,
    const std::string& outputFile)
{
    std::ofstream file(outputFile);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open output file " << outputFile << std::endl;
        return;
    }

    // Write header
    file << "Station ID,Station Name,Capacity,Optimal Inventory,Min UDF";
    int maxCapacity = 0;
    for (const auto& station : stations) {
        maxCapacity = std::max(maxCapacity, station.getCapacity());
    }

    for (int i = 0; i <= maxCapacity; i++) {
        file << ",UDF(" << i << ")";
    }
    file << std::endl;

    // Write data
    for (size_t s = 0; s < stations.size(); s++) {
        const Station& station = stations[s];
        file << station.getId() << ","
             << station.getName() << ","
             << station.getCapacity() << ","
             << optimalInventories[s] << ","
             << udfValues[s][optimalInventories[s]];

        for (int i = 0; i <= station.getCapacity(); i++) {
            file << "," << udfValues[s][i];
        }
        file << std::endl;
    }

    file.close();
}

int main(int argc, char* argv[])
{
    std::string inputFile = "../data/station_30min_intervals_with_capacity.csv";
    std::string outputFile = "results.csv";
    double discretizationLevel = 30.0; // 30-minute discretization

    if (argc > 1)
        inputFile = argv[1];
    if (argc > 2)
        outputFile = argv[2];
    if (argc > 3)
        discretizationLevel = std::stod(argv[3]);

    std::cout << "Loading stations from: " << inputFile << std::endl;
    std::cout << "Using discretization level: " << discretizationLevel << " minutes" << std::endl;

    std::vector<Station> stations = parseStationsFromCSV(inputFile);

    if (stations.empty()) {
        std::cerr << "No stations were loaded from the file." << std::endl;
        return 1;
    }

    std::cout << "Loaded " << stations.size() << " stations." << std::endl;

    std::vector<std::vector<double>> allUDFValues;
    std::vector<int> optimalInventories;
    allUDFValues.resize(stations.size());
    optimalInventories.resize(stations.size());
    std::filesystem::create_directories("results/intermediate");

// Process each station
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < static_cast<int>(stations.size()); i++) {
        UDFCalculator calculator;
        const Station& station = stations[i];
        try {
            std::string stationIdSanitized = station.getId();
            std::replace(stationIdSanitized.begin(), stationIdSanitized.end(), '/', '_'); // safe filename

            std::string stationResultFile = "results/intermediate/" + stationIdSanitized + ".csv";
            std::ifstream checkFile(stationResultFile);
            if (checkFile.good()) {
                std::cout << "Skipping already processed station: " << station.getName() << std::endl;
                continue;
            }
            auto start = std::chrono::steady_clock::now();
            std::vector<double> udfValues = calculator.calculateUDFForAllInventories(station, discretizationLevel);
            if (udfValues.empty()) {
                std::ostringstream warn;
                warn << "Warning: UDF values empty for station " << station.getName() << "\n";
#pragma omp critical
                std::cerr << warn.str();
            }

            int optimalInventory = calculator.findOptimalInventory(udfValues);
            // processing...
            auto end = std::chrono::steady_clock::now();
            std::ostringstream timing;
            timing << "Station: " << station.getName() << ", Time: "
                   << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
#pragma omp critical
            std::cout << timing.str();

            std::ofstream out(stationResultFile);
            if (!out.is_open()) {
                std::cerr << "Failed to write to " << stationResultFile << std::endl;
                continue;
            }

            out << "inventory,udf\n";
            for (int i = 0; i < udfValues.size(); i++) {
                out << i << "," << udfValues[i] << "\n";
            }
            out.close();
// Thread-safe write by index
#pragma omp critical
            {
                std::ostringstream ss;
                ss << "Processing station " << (i + 1) << "/" << stations.size()
                   << ": " << station.getName() << std::endl;
                std::cout << ss.str();
            }

            allUDFValues[i] = udfValues;
            optimalInventories[i] = optimalInventory;
        } catch (const std::exception& e) {
#pragma omp critical
            {
                std::cerr << "Error processing station: " << station.getId() << ": " << e.what() << std::endl;
            }
        }
    }
    // Write results to CSV
    writeCSVResults(stations, allUDFValues, optimalInventories, outputFile);
    std::cout << "Results written to: " << outputFile << std::endl;

    return 0;
}
