#ifndef GEOUTILS_H
#define GEOUTILS_H
#include <Arduino.h>
#include <math.h>

struct UTMCoordinates {
    double x;
    double y;
  };

  
#define WGS84_A 6378137.0 
#define WGS84_E2 0.00669438 
#define K0 0.9996

double calculateBearing(double lat1, double lon1, double lat2, double lon2);
UTMCoordinates LatLonToUTM(double lat, double lon);
double calculateDistance(UTMCoordinates location, UTMCoordinates destination);

#endif
