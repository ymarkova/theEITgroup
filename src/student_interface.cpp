#include "student_image_elab_interface.hpp"
#include "student_planning_interface.hpp"

#include <stdexcept>
#include <sstream>
#include <iterator>
#include <string> 

#include <cmath>
#include "plot.hpp"
#include "dubins.h"
#include "inflate_objects.hpp"
#include "vertical_cell_decomposition.hpp"

int enlarge = 600; // IF YOU CHANGE THIS CHANGE IT ALSO IN PLOT.CPP

namespace student {

  void loadImage(cv::Mat& img_out, const std::string& config_folder){  
    throw std::logic_error( "STUDENT FUNCTION - LOAD IMAGE - NOT IMPLEMENTED" );
  }

  void genericImageListener(const cv::Mat& img_in, std::string topic, const std::string& config_folder){
    throw std::logic_error( "STUDENT FUNCTION - IMAGE LISTENER - NOT CORRECTLY IMPLEMENTED" );
  }

  bool extrinsicCalib(const cv::Mat& img_in, std::vector<cv::Point3f> object_points, const cv::Mat& camera_matrix, cv::Mat& rvec, cv::Mat& tvec, const std::string& config_folder){
    throw std::logic_error( "STUDENT FUNCTION - EXTRINSIC CALIB - NOT IMPLEMENTED" );   
  }

  void imageUndistort(const cv::Mat& img_in, cv::Mat& img_out, const cv::Mat& cam_matrix, const cv::Mat& dist_coeffs, const std::string& config_folder){
    throw std::logic_error( "STUDENT FUNCTION - IMAGE UNDISTORT - NOT IMPLEMENTED" );  
  }

  void findPlaneTransform(const cv::Mat& cam_matrix, const cv::Mat& rvec, 
                          const cv::Mat& tvec, const std::vector<cv::Point3f>& object_points_plane, 
                          const std::vector<cv::Point2f>& dest_image_points_plane, 
                          cv::Mat& plane_transf, const std::string& config_folder){
    throw std::logic_error( "STUDENT FUNCTION - FIND PLANE TRANSFORM - NOT IMPLEMENTED" );  
  }


  void unwarp(const cv::Mat& img_in, cv::Mat& img_out, const cv::Mat& transf, const std::string& config_folder){
    throw std::logic_error( "STUDENT FUNCTION - UNWRAP - NOT IMPLEMENTED" );   
  }

  bool processMap(const cv::Mat& img_in, const double scale, std::vector<Polygon>& obstacle_list, std::vector<std::pair<int,Polygon>>& victim_list, Polygon& gate, const std::string& config_folder){
    throw std::logic_error( "STUDENT FUNCTION - PROCESS MAP - NOT IMPLEMENTED" );   
  }

  bool findRobot(const cv::Mat& img_in, const double scale, Polygon& triangle, double& x, double& y, double& theta, const std::string& config_folder){
    throw std::logic_error( "STUDENT FUNCTION - FIND ROBOT - NOT IMPLEMENTED" );    
  }

  void reset_obs_plot(cv::Mat plot,std::vector< std::vector<POINT> > obstacles){
      for (unsigned i=0; i<obstacles.size(); i++) {
          for(unsigned j=1; j<obstacles[i].size();j++){
              cv::line(plot, cv::Point2f(obstacles[i][j-1].x*enlarge,obstacles[i][j-1].y*enlarge), cv::Point2f(obstacles[i][j].x*enlarge,obstacles[i][j].y*enlarge), cv::Scalar(255,255,255), 2);
              cv::line(plot, cv::Point2f(obstacles[i][j-1].x*enlarge,obstacles[i][j-1].y*enlarge), cv::Point2f(obstacles[i][j].x*enlarge,obstacles[i][j].y*enlarge), cv::Scalar(210,210,210), 1);
              if (j == obstacles[i].size() -1){
                  cv::line(plot, cv::Point2f(obstacles[i][j].x*enlarge,obstacles[i][j].y*enlarge), cv::Point2f(obstacles[i][0].x*enlarge,obstacles[i][0].y*enlarge), cv::Scalar(255,255,255), 2);
                  cv::line(plot, cv::Point2f(obstacles[i][j].x*enlarge,obstacles[i][j].y*enlarge), cv::Point2f(obstacles[i][0].x*enlarge,obstacles[i][0].y*enlarge), cv::Scalar(210,210,210), 1);        
              }
          }
      }     
  }

  bool planPath(const Polygon& borders, const std::vector<Polygon>& obstacle_list,
                const std::vector<Polygon>& gate_list,
                const std::vector<float> x, const std::vector<float> y, const std::vector<float> theta,
                std::vector<Path>& path, const std::string& config_folder){

    //initialising the plot
    int l = 1000;        
    cv::Mat plot(l - 300,l, CV_8UC3, cv::Scalar(255,255,255));


    //OBSTACLES PREPROCESSING
    
    // inflating the obsticales and borders of the arena
    float inflate_value = 30;
    std::vector<Polygon> inflated_obstacle_list = inflate_obstacles(obstacle_list,inflate_value,plot);
    const Polygon inflated_borders = inflate_borders(borders,-inflate_value,plot);

    //Basem what is this exactly doing and why do we do it twice?
    inflated_obstacle_list =  trim_obstacles(inflated_obstacle_list,inflated_borders, plot);
    inflated_obstacle_list =  trim_obstacles(inflated_obstacle_list,inflated_borders, plot);
    inflated_obstacle_list =  merge_obstacles (inflated_obstacle_list, plot);

    // TO DO: implement a function to merge the obstacles that are over lapping
    // TO DO: delete all the vertcices outside of the borders


    //ADJUSTING THE FORMAT OF THE INPUT DATA
    
    //convert input boundary data into the data format we use 
    std::vector<POINT> boundary;
    for (const auto &position : inflated_borders) {
      boundary.push_back({position.x,position.y});
    }
 
    //convert input start points data into the data format we use 
    std::vector<POINT> start_point;
    for (int i = 0; i < x.size(); i++) {
      start_point.push_back(POINT{x[i],y[i],theta[i]});
    }   
    
    //convert input gate position data into the data format we use
    std::vector<POINT> gate;
    //end point is the center of the gate 
    std::vector<POINT> end_point;
    for (int i = 0; i < gate_list.size(); i++) {
      for (const auto &position : gate_list[i]) {
        gate.push_back({position.x,position.y});
      }
      end_point.push_back(centroid(gate));
      gate.clear();
    }

    //the total number of vertices of all obstacles together
    int vertices_num = 0;

    //convert input obstacles data into the data format we use    
    std::vector< std::vector<POINT> > obstacles;
    std::vector<POINT> obstacle;
    for (int i = 0; i < inflated_obstacle_list.size(); i++) {
      for (const auto &position : inflated_obstacle_list[i]) {
        obstacle.push_back(POINT{position.x,position.y,-1,i});
        vertices_num += 1;
      }
      obstacles.push_back(obstacle);
      obstacle.clear();
    }


    //ROAD-MAP CONSTRUCTION

    //sorting obstacle vertices by their x value in increasing order
    std::vector<POINT> sorted_vertices;    
    sorted_vertices = sort_vertices(obstacles, sorted_vertices, vertices_num); 

    //adding the first point of the obstacle to the end to close the polygon
    obstacles = close_polygons(obstacles);

    //determining the limits of the vertical lines
    float y_limit_lower = min(min(boundary[0].y, boundary[1].y), min(boundary[2].y, boundary[3].y));
    float y_limit_upper = max(max(boundary[0].y, boundary[1].y), max(boundary[2].y, boundary[3].y));

    //finding the vertical lines
    std::vector< SEGMENT > open_line_segments;
    open_line_segments = find_lines(sorted_vertices, obstacles, y_limit_lower, y_limit_upper);

    //finding basic cells
    std::vector< std::vector<POINT> > cells;
    cells = find_cells(open_line_segments, sorted_vertices, obstacles);

    //merging overlaping polygons
    cells = merge_polygons(cells);

    //adding cells from boundary lines to closest obstacle
    cells = boundary_cells(boundary, cells, sorted_vertices, y_limit_lower, y_limit_upper);

    //getting the graph edges & vertices
    std::vector<POINT> graph_edges;
    std::vector<POINT> graph_vertices;
    tie(graph_edges, graph_vertices) = get_graph(cells);
    
    //adding the start and end point for each robot into the graph
    tie(graph_edges, graph_vertices) = add_start_end(graph_vertices, graph_edges, start_point, end_point, obstacles);

    //constructing the graph
    std::vector< std::vector<int> > graph;
    graph = graph_construction(graph_vertices, graph_edges);
    
    //finding a path using breadth first search
    std::vector<int> my_path;
    my_path = bfs(graph, graph_vertices.size()-2, graph_vertices.size()-1);

    //separating only the graph vertices which belong to the path for optimization purposes
    std::vector<POINT> new_graph_vertices;
    for(int i = 0 ; i< my_path.size();i++){
      new_graph_vertices.push_back({graph_vertices[my_path[i]].x,graph_vertices[my_path[i]].y});
    }

    //optimizing the graph
    std::vector< std::vector<int> >  optimized_graph;
    optimized_graph = optimize_graph(my_path, new_graph_vertices, obstacles);

    //calculating the optimized path using breadth first search
    std::vector<int> optimized_path;
    optimized_path = bfs(optimized_graph, 0, new_graph_vertices.size()-1);

    //changing the path index to actual points for dubins
    std::vector<robotPos> path_points;
    path_points = index_to_coordinates(optimized_path, new_graph_vertices);

    //printing and plotting the results
    print_data(boundary, start_point, end_point, obstacles, graph_vertices, graph, new_graph_vertices, optimized_graph, my_path, optimized_path);
    plot_map(plot, sorted_vertices, cells, start_point, end_point, graph, graph_vertices, new_graph_vertices, optimized_path, my_path); 
    

    //DUBINS PATH

    float Kmax = 5.0;

    // A fake path from roadmap
    robotPos pos0 = {x[0], y[0], theta[0]};
    robotPos pos1 = {0.8, 0.6, -M_PI/6};
    robotPos pos2 = {1.3, 0.96, 0};

    std::vector<robotPos> rmPos = {pos0, pos1, pos2};

    // Compute the dubins path between two adjacent points
    for (auto it0 = rmPos.begin(), it1 = std::next(rmPos.begin());
         it0 != std::prev(rmPos.end()) && it1 != rmPos.end(); ++it0, ++it1)
    {
      shortestDubinsResult result = dubinsShortestPath(*it0, *it1, Kmax);

      if (result.pidx > -1){
        for (auto it = result.dubinsWPList.begin(); it != result.dubinsWPList.end(); ++it){
          path[0].points.emplace_back((*it).s, (*it).pos.x, (*it).pos.y, (*it).pos.th, (*it).k);
        }
      }
    }

    //END OF DUBBINS PATH

    return true;
  }
}