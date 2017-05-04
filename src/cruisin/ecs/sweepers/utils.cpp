#include "utils.h"
#include <math.h>


//--------------------------itos------------------------------------
//	converts an integer to a string
//------------------------------------------------------------------		
std::string itos(int arg) {
  std::ostringstream buffer;

  //send the int to the ostringstream
  buffer << arg;

  //capture the string
  return buffer.str();
}


//--------------------------ftos------------------------------------
//	converts a float to a string
//------------------------------------------------------------------		
std::string ftos(float arg) {
  std::ostringstream buffer;

  //send the int to the ostringstream
  buffer << arg;

  //capture the string
  return buffer.str();
}
//-------------------------------------Clamp()-----------------------------------------
//
//	clamps the first argument between the second two
//
//-------------------------------------------------------------------------------------
void Clamp(double &arg, double min, double max) {
  if (arg < min) {
    arg = min;
  }

  if (arg > max) {
    arg = max;
  }
}
