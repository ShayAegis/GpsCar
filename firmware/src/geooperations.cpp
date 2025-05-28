#include "geoutils.h"

double calculateDistance(UTMCoordinates location, UTMCoordinates destination) {
    double x1=location.x;
    double y1=location.y;
    double x2=destination.x;
    double y2=destination.y;
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
  }

double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
    lat1 = radians(lat1);
    lon1 = radians(lon1);
    lat2 = radians(lat2);
    lon2 = radians(lon2);
  
    double dLon = lon2 - lon1;
  
    double y = sin(dLon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
  
    double bearing = atan2(y, x);
    bearing = degrees(bearing);
    bearing = fmod((bearing + 360), 360);
  
    return bearing;
  }
  

UTMCoordinates LatLonToUTM(double lat, double lon) {
    int zoneNumber = 18;
    double lonOrigin = (zoneNumber - 1) * 6 - 180 + 3;
  
    double latRad = lat * M_PI / 180.0;
    double lonRad = lon * M_PI / 180.0;
    double lonOriginRad = lonOrigin * M_PI / 180.0;
  
    double N = WGS84_A / sqrt(1 - WGS84_E2 * sin(latRad) * sin(latRad));
    double T = tan(latRad) * tan(latRad);
    double C = WGS84_E2 / (1 - WGS84_E2) * cos(latRad) * cos(latRad);
    double A = cos(latRad) * (lonRad - lonOriginRad);
  
    double M = WGS84_A * (
        (1 - WGS84_E2/4 - 3*WGS84_E2*WGS84_E2/64 - 5*WGS84_E2*WGS84_E2*WGS84_E2/256) * latRad
        - (3*WGS84_E2/8 + 3*WGS84_E2*WGS84_E2/32 + 45*WGS84_E2*WGS84_E2*WGS84_E2/1024) * sin(2*latRad)
        + (15*WGS84_E2*WGS84_E2/256 + 45*WGS84_E2*WGS84_E2*WGS84_E2/1024) * sin(4*latRad)
        - (35*WGS84_E2*WGS84_E2*WGS84_E2/3072) * sin(6*latRad)
    );
  
    double utmX = K0 * N * (A + (1-T+C)*pow(A,3)/6 + (5-18*T+T*T+72*C-58*0.006739496742)*pow(A,5)/120) + 500000.0;
    double utmY = K0 * (M + N * tan(latRad) * (A*A/2 + (5-T+9*C+4*C*C)*pow(A,4)/24 + (61-58*T+T*T+600*C-330*0.006739496742)*pow(A,6)/720));
  
    if (lat < 0) {
      utmY += 10000000.0;
    }
  
    UTMCoordinates result;
    result.x = utmX;
    result.y = utmY;
    return result;
  }