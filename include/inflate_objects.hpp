#include "structs.h"
#include "collision_detect.hpp"
#include "clipper/clipper.hpp"


Polygon simplify_poly (Polygon poly, float tol);
std::vector<Polygon> inflate_obstacles(const std::vector<Polygon> &obstacle_list,float inflate_value,bool simplify, cv::Mat plot);
Polygon inflate_borders(const Polygon &borders, float inflate_value, cv::Mat plot);
std::vector<Polygon> trim_obstacles(const std::vector<Polygon>& obstacle_list,const Polygon &borders, cv::Mat plot);
std::vector<Polygon> merge_obstacles(const std::vector<Polygon>& obstacle_list,bool simplify, cv::Mat plot);



