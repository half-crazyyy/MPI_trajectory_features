#include "csv_reader.hpp"

#include <cmath>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

int category_code(const std::string& name) {
    static const std::unordered_map<std::string, int> codes = {
        {"plane", 0}, {"drone", 1}, {"helicopter", 2}, {"balloon", 3}
    };
    auto it = codes.find(name);
    if (it == codes.end()) {
        throw std::runtime_error("Unknown category: " + name);
    }
    return it->second;
}

std::string category_name(int category) {
    switch (category) {
    case 0: return "plane";
    case 1: return "drone";
    case 2: return "helicopter";
    case 3: return "balloon";
    default: return "unknown";
    }
}

std::vector<TrajectoryPoint> read_csv_points(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open input CSV: " + path);
    }

    std::vector<TrajectoryPoint> points;
    std::string line;

    // Первая строка считается заголовком c category, record_index, element_index, v и h
    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string token;
        TrajectoryPoint point;

        std::getline(ss, token, ',');
        point.category = category_code(token);

        std::getline(ss, token, ',');
        point.record_index = std::stoi(token);

        std::getline(ss, token, ',');
        point.element_index = std::stoi(token);

        std::getline(ss, token, ',');
        point.velocity = std::stod(token);

        std::getline(ss, token, ',');
        point.height = std::stod(token);

        points.push_back(point);
    }

    return points;
}

void write_sample_dataset(const std::string& path, int trajectories_per_class, int points_per_trajectory) {
    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot create sample dataset: " + path);
    }

    std::mt19937 rng(42);
    std::normal_distribution<double> noise(0.0, 1.0);

    out << "category,record_index,element_index,v,h\n";

    for (int category = 0; category < 4; ++category) {
        for (int record = 0; record < trajectories_per_class; ++record) {
            const double base_v = 35.0 + category * 45.0 + (record % 7) * 1.5;
            const double base_h = 200.0 + category * 900.0 + (record % 11) * 10.0;

            for (int point = 0; point < points_per_trajectory; ++point) {
                const double t = static_cast<double>(point) / std::max(1, points_per_trajectory - 1);
                const double velocity = base_v + 8.0 * std::sin(6.28318 * t) + noise(rng);
                const double height = base_h + 120.0 * t + 25.0 * std::cos(6.28318 * t) + noise(rng) * 4.0;

                out << category_name(category) << ','
                    << record << ','
                    << point << ','
                    << velocity << ','
                    << height << '\n';
            }
        }
    }
}
