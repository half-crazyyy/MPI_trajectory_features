#ifndef CSV_READER_HPP
#define CSV_READER_HPP

#include <string>
#include <vector>

// Одна точка траектории из входного CSV-файла
struct TrajectoryPoint {
    int category = 0;
    int record_index = 0;
    int element_index = 0;
    double velocity = 0.0;
    double height = 0.0;
};

std::vector<TrajectoryPoint> read_csv_points(const std::string& path);
void write_sample_dataset(const std::string& path, int trajectories_per_class, int points_per_trajectory);
std::string category_name(int category);
int category_code(const std::string& name);

#endif // CSV_READER_HPP
