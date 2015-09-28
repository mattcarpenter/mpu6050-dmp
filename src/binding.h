#include <nan.h>
#include <pthread.h>
#include <chrono>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

NAN_METHOD(getAttitude);
NAN_METHOD(getRotation);
NAN_METHOD(initialize);
