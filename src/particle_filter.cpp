/**
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "helper_functions.h"

using std::string;
using std::vector;
using namespace std;

inline double dist(double x1, double y1, double x2, double y2);

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * TODO: Set the number of particles. Initialize all particles to 
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1. 
   * TODO: Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method 
   *   (and others in this file).
   */
  num_particles = 50 ;  // TODO: Set the number of particles
  default_random_engine gen;

  if(is_initialized){
    return ;
  }
  int i = 0;
  for(;i<num_particles;)
  {
     normal_distribution<double> dist_x(x, std[0]);
     normal_distribution<double> dist_y(y, std[1]);
     normal_distribution<double> dist_theta(theta, std[2]);
     
     Particle particle;
     particle.id = i;
     particle.x = dist_x(gen);
     particle.y = dist_y(gen);
     particle.theta = dist_theta(gen);
     particle.weight = 1.0;
     particles.push_back(particle);
    
     i += 1;

  }
  cout<<"Test";
  is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], 
                                double velocity, double yaw_rate) {
  /**
   * TODO: Add measurements to each particle and add random Gaussian noise.
   * NOTE: When adding noise you may find std::normal_distribution 
   *   and std::default_random_engine useful.
   *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
   *  http://www.cplusplus.com/reference/random/default_random_engine/
   */
 if(yaw_rate !=0) 
    {
      for(auto &particle: particles){
        double x = particle.x;
        double y = particle.y;
        double theta = particle.theta;

        particle.x = x + (velocity/yaw_rate)* (sin(theta + yaw_rate*delta_t) - sin(theta));
        particle.y = y + (velocity/yaw_rate)*(cos(theta) - cos(theta + yaw_rate*delta_t));
        particle.theta = theta + yaw_rate*delta_t;
      }
    }
}

void ParticleFilter::dataAssociation(vector<LandmarkObs> predicted, 
                                     vector<LandmarkObs>& observations) {
  /**
   * TODO: Find the predicted measurement that is closest to each 
   *   observed measurement and assign the observed measurement to this 
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will 
   *   probably find it useful to implement this method and use it as a helper 
   *   during the updateWeights phase.
   */
  for(auto &obs: observations){
    double minm_distance = -1.0;
    double minm_id = -1;
    for(auto pred: predicted){
      double distance = dist(obs.x, obs.y, pred.x, pred.y);

      if(distance < minm_distance){
        minm_distance = distance;
        minm_id = pred.id;
      }

    }
    obs.id = minm_id;
  }
}
void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
                                   const vector<LandmarkObs> &observations, 
                                   const Map &map_landmarks) {
  /**
   * TODO: Update the weights of each particle using a mult-variate Gaussian 
   *   distribution. You can read more about this distribution here: 
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system. 
   *   Your particles are located according to the MAP'S coordinate system. 
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */

  double sig_x = 0.15;
  double sig_y = 0.15;
  double norm = 1/(2*M_PI*sig_x*sig_y);

  for(auto &particle: particles){
    vector<LandmarkObs> predicted{};

    double xp = particle.x;
    double yp = particle.y;
    double theta = particle.theta;

    for(auto &map_landmark: map_landmarks.landmark_list){
      int id = map_landmark.id_i;
      double x = map_landmark.x_f;
      double y = map_landmark.y_f;

      if (dist(xp, yp, x, y) < sensor_range){
        predicted.push_back(LandmarkObs{id, x, y});
      }
    }
    vector<LandmarkObs> modified_observations;
    for(auto &observation: observations){
      int id = observation.id;
      double xc = observation.x;
      double yc = observation.y;

      double xm = xp + cos(theta)*xc - sin(theta)*yc;
      double ym = yp + sin(theta)*xc + cos(theta)*yc;

      modified_observations.push_back(LandmarkObs{id, xm, ym});
    }
    dataAssociation(predicted, modified_observations);

    double total_prob = 1.0;
    for(auto &obs: modified_observations){
      auto pred = predicted[obs.id];
      auto x_delta = obs.x - pred.x;
      auto y_delta = obs.y - pred.y;
      total_prob *= norm * exp(-((x_delta*x_delta)/(2*sig_x*sig_x) + (y_delta*y_delta)/(2*sig_y*sig_y)));
    }
    particle.weight = total_prob;
    weights.push_back(total_prob);
  }
}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional 
   *   to their weight. 
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */
  vector<Particle> new_particles(num_particles);
  random_device rd;
  default_random_engine gen(rd());
  discrete_distribution<int> dist_weight(weights.begin(), weights.end());

  for(int i=0;i<num_particles;i++){
    new_particles[i] = particles[dist_weight(gen)];
  }
  particles = move(new_particles);
}

void ParticleFilter::SetAssociations(Particle& particle, 
                                     const vector<int>& associations, 
                                     const vector<double>& sense_x, 
                                     const vector<double>& sense_y) {
  // particle: the particle to which assign each listed association, 
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates
  particle.associations= associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}
