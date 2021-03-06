#include "catch.hpp"
#include "../src/helper_functions.h"
#include "../src/particle_filter.h"
#include <iostream>
#include <cmath>

using namespace std;

TEST_CASE("Particle filter initialized according to GPS position", "[init]") {
  const double xGps = 1.0, yGps = 2.0, thetaGps = 3.0;
  const double xGpsStd = 0.1, yGpsStd = 0.2, thetaGpsStd = 0.3;
  ParticleFilter pf;
  double stdevGps[] = {xGpsStd, yGpsStd, thetaGpsStd};

  pf.init(xGps, yGps, thetaGps, stdevGps);

  // Calculate mean values.
  double xSum = 0.0, ySum = 0.0, thetaSum = 0.0;
  for (int i = 0; i < pf.particles.size(); ++i) {
    xSum += pf.particles[i].x;
    ySum += pf.particles[i].y;
    thetaSum += pf.particles[i].theta;
  }
  const double xMean = xSum / (double)pf.particles.size();
  const double yMean = ySum / (double)pf.particles.size();
  const double thetaMean = thetaSum / (double)pf.particles.size();

  // Verify that mean values are roughly within expectations.
  REQUIRE(xMean == Approx(xGps).margin(xGpsStd * 0.1));
  REQUIRE(yMean == Approx(yGps).margin(yGpsStd * 0.1));
  REQUIRE(thetaMean == Approx(thetaGps).margin(thetaGpsStd * 0.1));

  // Calculate standard deviations
  double xSqDevSum = 0.0, ySqDevSum = 0.0, thetaSqDevSum = 0.0;
  for (int i = 0; i < pf.particles.size(); ++i) {
    xSqDevSum += pow(pf.particles[i].x - xMean, 2.0);
    ySqDevSum += pow(pf.particles[i].y - yMean, 2.0);
    thetaSqDevSum += pow(pf.particles[i].theta - thetaMean, 2.0);
  }
  const double xStdev = sqrt(xSqDevSum / (double)pf.particles.size());
  const double yStdev = sqrt(ySqDevSum / (double)pf.particles.size());
  const double thetaStdev = sqrt(thetaSqDevSum / (double)pf.particles.size());

  // Verify that standard deviations are roughly within expectations.
  REQUIRE(xStdev == Approx(xGpsStd).margin(xGpsStd * 0.1));
  REQUIRE(yStdev == Approx(yGpsStd).margin(yGpsStd * 0.1));
  REQUIRE(thetaStdev == Approx(thetaGpsStd).margin(thetaGpsStd * 0.1));
}

TEST_CASE("Particle filter prediction when driving straight", "[prediction]") {

  const double delta_t = 1.0;
  double std_pos[] = {0.0, 0.0, 0.0}; // No noise
  const double velocity = 10.0;
  const double yaw_rate = 0.0; // Driving straight

  ParticleFilter pf;
  REQUIRE(pf.particles.size() == 0);

  typedef struct {
    Particle in;
    Particle expected;
  } TestElement;

  vector<TestElement> testVector;

  // Driving east
  testVector.push_back({
    .in =       {.id = 0, .x = 100.0, .y = 100.0, .theta = 0.0},
    .expected = {.id = 0, .x = 110.0, .y = 100.0, .theta = 0.0}
  });

  // Driving west
  testVector.push_back({
    .in =       {.id = 0, .x = 100.0, .y = 100.0, .theta = M_PI},
    .expected = {.id = 0, .x =  90.0, .y = 100.0, .theta = M_PI}
  });

  // Driving north
  testVector.push_back({
    .in =       {.id = 0, .x = 100.0, .y = 100.0, .theta = M_PI / 2.0},
    .expected = {.id = 0, .x = 100.0, .y = 110.0, .theta = M_PI / 2.0}
  });

  // Driving south
  testVector.push_back({
    .in =       {.id = 0, .x = 100.0, .y = 100.0, .theta = -M_PI / 2.0},
    .expected = {.id = 0, .x = 100.0, .y =  90.0, .theta = -M_PI / 2.0}
  });

  // Driving south-east
  testVector.push_back({
    .in =       {.id = 0, .x = 100.00, .y = 100.00, .theta = -M_PI / 4.0},
    .expected = {.id = 0, .x = 107.07, .y =  92.93, .theta = -M_PI / 4.0}
  });

  for (auto testElement = testVector.begin(); testElement != testVector.end(); ++testElement) {
    pf.particles.push_back(testElement->in);
  }

  pf.prediction(delta_t, std_pos, velocity, yaw_rate);

  for (int i = 0; i < testVector.size(); ++i) {
    REQUIRE(pf.particles[i].x == Approx(testVector[i].expected.x));
    REQUIRE(pf.particles[i].y == Approx(testVector[i].expected.y));
    REQUIRE(pf.particles[i].theta == Approx(testVector[i].expected.theta));
  }
}

TEST_CASE("Particle filter prediction when turning", "[prediction]") {
  /* Test data from lesson example on the prediction step:
   * Given the car’s last position was at (102 m, 65 m) with a heading of
   * (5pi)/8 radians, the car’s velocity was 110 m / s, and the car’s yaw rate
   * was pi/8 rad / s over the last 0.1 seconds, what is the car’s new position
   * and heading? Answer: (x, y, theta) = (97.59, 75.08, (51pi)/80)
   *
   * Trivia: Although a speed of 110 m/s is possible with some hypercars, it's
   * certainly impossible when going at a yaw-rate of pi/8, which would produce
   * an centrifugal force of 28.5G.
   */
  ParticleFilter pf;
  REQUIRE(pf.particles.size() == 0);

  pf.particles.push_back({.id=0, .x=102.0, .y=65.0, .theta=(5.0 * M_PI / 8.0)});
  const double delta_t = 0.1;
  double std_pos[] = {0.0, 0.0, 0.0}; // No noise
  const double velocity = 110.0;
  const double yaw_rate = M_PI / 8.0;

  pf.prediction(delta_t, std_pos, velocity, yaw_rate);

  REQUIRE(pf.particles[0].x == Approx(97.59).margin(0.005));
  REQUIRE(pf.particles[0].y == Approx(75.08).margin(0.005));
  REQUIRE(pf.particles[0].theta == Approx(51.0 * M_PI / 80.0).margin(0.005));
}

TEST_CASE("Particle filter prediction adds noise", "[prediction]") {
  // Initialize
  const double xGps = 1.0, yGps = 2.0, thetaGps = 3.0;
  const double xGpsStd = 0.1, yGpsStd = 0.2, thetaGpsStd = 0.3;
  ParticleFilter pf;
  double stdevGps[] = {xGpsStd, yGpsStd, thetaGpsStd};
  pf.init(xGps, yGps, thetaGps, stdevGps);

  // Predict
  const double delta_t = 0.1;
  const double xPosStd = 0.4, yPosStd = 0.5, thetaPosStd = 0.6;
  double stdevPos[] = {xPosStd, yPosStd, thetaPosStd};
  const double velocity = 0.0;
  const double yaw_rate = 1.0;
  pf.prediction(delta_t, stdevPos, velocity, yaw_rate);

  // Calculate mean values.
  double xSum = 0.0, ySum = 0.0, thetaSum = 0.0;
  for (int i = 0; i < pf.particles.size(); ++i) {
    xSum += pf.particles[i].x;
    ySum += pf.particles[i].y;
    thetaSum += pf.particles[i].theta;
  }
  const double xMean = xSum / (double)pf.particles.size();
  const double yMean = ySum / (double)pf.particles.size();
  const double thetaMean = thetaSum / (double)pf.particles.size();

  // Verify that mean values are roughly within expectations.
  REQUIRE(xMean == Approx(xGps).margin(xGpsStd * 0.3));
  REQUIRE(yMean == Approx(yGps).margin(yGpsStd * 0.3));
  REQUIRE(thetaMean == Approx(thetaGps).margin(thetaGpsStd * 0.3));

  // Calculate standard deviations
  double xSqDevSum = 0.0, ySqDevSum = 0.0, thetaSqDevSum = 0.0;
  for (int i = 0; i < pf.particles.size(); ++i) {
    xSqDevSum += pow(pf.particles[i].x - xMean, 2.0);
    ySqDevSum += pow(pf.particles[i].y - yMean, 2.0);
    thetaSqDevSum += pow(pf.particles[i].theta - thetaMean, 2.0);
  }
  const double xStdev = sqrt(xSqDevSum / (double)pf.particles.size());
  const double yStdev = sqrt(ySqDevSum / (double)pf.particles.size());
  const double thetaStdev = sqrt(thetaSqDevSum / (double)pf.particles.size());

  // Verify that standard deviations are roughly within expectations.
  /* TODO: not sure why standard deviations adds up? 
   * it should be the variances that adds up.
   */
  REQUIRE(xStdev == Approx(xGpsStd + xPosStd).margin((xGpsStd + xPosStd) * 0.1));
  REQUIRE(yStdev == Approx(yGpsStd + yPosStd).margin((yGpsStd + yPosStd) * 0.1));
  REQUIRE(thetaStdev == Approx(thetaGpsStd + thetaPosStd).margin((thetaGpsStd + thetaPosStd) * 0.1));
}

TEST_CASE("Particle filter data association", "[updateweights]") {
  Map map;
  map.landmark_list.push_back({.id_i = 0, .x_f = 10.0, .y_f = 20.0});
  map.landmark_list.push_back({.id_i = 1, .x_f = 10.0, .y_f = 30.0});
  map.landmark_list.push_back({.id_i = 2, .x_f = 30.0, .y_f = 10.0});
  map.landmark_list.push_back({.id_i = 3, .x_f = 30.0, .y_f = 20.0});

  std::vector<TransformedObservation> observations;
  observations.push_back(TransformedObservation(31.0, 10.0));
  observations.push_back(TransformedObservation(29.0, 25.0));
  observations.push_back(TransformedObservation( 0.0, 0.0));

  const int expected[] = {2, 3, 0};

  ParticleFilter::dataAssociation(map, observations);

  for (int i = 0; i < observations.size(); ++i) {
    REQUIRE(observations[i].landmarkIdx == expected[i]);
  }
}

TEST_CASE("Particle filter data association without landmarks", "[updateweights]") {
  Map map;  // empty
  std::vector<TransformedObservation> observations;
  observations.push_back(TransformedObservation(31.0, 10.0));
  observations.push_back(TransformedObservation(29.0, 25.0));
  observations.push_back(TransformedObservation(0.0, 0.0));

  const int expected = TransformedObservation::invalidLandmarkIdx; // no valid association
  ParticleFilter::dataAssociation(map, observations);

  for (int i = 0; i < observations.size(); ++i) {
    REQUIRE(observations[i].landmarkIdx == expected);
  }
}


TEST_CASE("Particle filter update weights", "[updateweights]") {
  /* Test data from lesson examples on particle weight calculation. */
  ParticleFilter pf;
  pf.particles.push_back({.id = 0, .x = 4.0, .y = 5.0, .theta = -M_PI / 2.0, .weight = 1.0});

  vector<Observation> observations;
  observations.push_back({.x = 2.0, .y = 2.0});
  observations.push_back({.x = 3.0, .y = -2.0});
  observations.push_back({.x = 0.0, .y = -4.0});

  const double sensor_range = 50;
  const double std_landmark[] = {0.3, 0.3};

  Map map;
  map.landmark_list.push_back({.id_i = 1, .x_f = 5.0, .y_f = 3.0});
  map.landmark_list.push_back({.id_i = 2, .x_f = 2.0, .y_f = 1.0});
  map.landmark_list.push_back({.id_i = 3, .x_f = 6.0, .y_f = 1.0});
  map.landmark_list.push_back({.id_i = 4, .x_f = 7.0, .y_f = 4.0});
  map.landmark_list.push_back({.id_i = 5, .x_f = 4.0, .y_f = 7.0});

  const double expected = 4.59112934464959e-53;

  pf.updateWeights(sensor_range, std_landmark, observations, map);

  REQUIRE(pf.particles[0].weight == Approx(expected).margin(1.0e-55));

  REQUIRE(pf.particles[0].associations.size() == 3);
  REQUIRE(pf.particles[0].sense_x.size() == 3);
  REQUIRE(pf.particles[0].sense_y.size() == 3);

  REQUIRE(pf.particles[0].associations[0] == 1);
  REQUIRE(pf.particles[0].sense_x[0] == Approx(6.0));
  REQUIRE(pf.particles[0].sense_y[0] == Approx(3.0));
  REQUIRE(pf.particles[0].associations[1] == 2);
  REQUIRE(pf.particles[0].sense_x[1] == Approx(2.0));
  REQUIRE(pf.particles[0].sense_y[1] == Approx(2.0));
  REQUIRE(((pf.particles[0].associations[2] == 2) ||
           (pf.particles[0].associations[2] == 5)));
  REQUIRE(pf.particles[0].sense_x[2] == Approx(0.0));
  REQUIRE(pf.particles[0].sense_y[2] == Approx(5.0));
}

TEST_CASE("Particle filter update weights without a map", "[updateweights]") {
  /* Test data from lesson examples on particle weight calculation. */
  ParticleFilter pf;
  pf.particles.push_back({.id = 0, .x = 4.0, .y = 5.0, .theta = -M_PI / 2.0, .weight = 1.0});

  vector<Observation> observations;
  observations.push_back({.x = 2.0, .y = 2.0});

  const double sensor_range = 50;
  const double std_landmark[] = {0.3, 0.3};

  Map map; // Empty map

  // Expects very small particle weight without landmarks to observe.
  const double expected = 0.0;

  pf.updateWeights(sensor_range, std_landmark, observations, map);
  REQUIRE(pf.particles[0].weight == Approx(expected).margin(1.0e-100));
}


TEST_CASE("Particle filter resample should update the distribution based on particle weights", "[resample]") {
  ParticleFilter pf;
  pf.particles.push_back({.id = 0, .x = 0.0, .y = 0.0, .theta = 0.0, .weight = 1.0});
  pf.particles.push_back({.id = 1, .x = 0.0, .y = 0.0, .theta = 0.0, .weight = 1.0});
  pf.particles.push_back({.id = 2, .x = 0.0, .y = 0.0, .theta = 0.0, .weight = 1.0});
  pf.particles.push_back({.id = 3, .x = 0.0, .y = 0.0, .theta = 0.0, .weight = 1.0});
  pf.particles.push_back({.id = 4, .x = 0.0, .y = 0.0, .theta = 0.0, .weight = 1.0});
  pf.particles.push_back({.id = 5, .x = 0.0, .y = 0.0, .theta = 0.0, .weight = 25.0});
  pf.particles.push_back({.id = 6, .x = 0.0, .y = 0.0, .theta = 0.0, .weight = 1.0});
  pf.particles.push_back({.id = 7, .x = 0.0, .y = 0.0, .theta = 0.0, .weight = 1.0});
  pf.particles.push_back({.id = 8, .x = 0.0, .y = 0.0, .theta = 0.0, .weight = 1.0});
  pf.particles.push_back({.id = 9, .x = 0.0, .y = 0.0, .theta = 0.0, .weight = 1.0});
  pf.particles.push_back({.id = 10, .x = 0.0, .y = 0.0, .theta = 0.0, .weight = 75.0});

  // Expects more samples of particle id 5 (weight 25) and particle id 10 (weight 75).
  const int expectedMinCount[] = {0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 4};
  const int expectedMaxCount[] = {2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 7};

  pf.resample();

  int actualCount[11] = {0};
  for (auto particle = pf.particles.begin(); particle != pf.particles.end(); ++particle) {
    actualCount[particle->id]++;
  }

  for (int i = 0; i < sizeof(actualCount) / sizeof(actualCount[0]); ++i) {
    REQUIRE(actualCount[i] >= expectedMinCount[i]);
    REQUIRE(actualCount[i] <= expectedMaxCount[i]);
  }
}
