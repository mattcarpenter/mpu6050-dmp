#include <node.h>
#include <v8.h>
#include <pthread.h>
#include <chrono>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

using namespace v8;
using namespace std::chrono;

pthread_t readThread;
MPU6050 mpu;

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

float data_out[6];

void *readFromFIFO(void *ypr_void_ptr); 
void initMPU();

Handle<Value> GetAttitude(const Arguments& args) {
  HandleScope scope;
  Local<Object> obj = Object::New();
  obj->Set(String::NewSymbol("roll"), Number::New(data_out[1]));
  obj->Set(String::NewSymbol("pitch"), Number::New(data_out[2]));
  obj->Set(String::NewSymbol("yaw"), Number::New(data_out[0]));
  return scope.Close(obj);
}

Handle<Value> GetRotation(const Arguments& args) {
  HandleScope scope;
  Local<Object> obj = Object::New();
  obj->Set(String::NewSymbol("roll"), Number::New(data_out[3]));
  obj->Set(String::NewSymbol("pitch"), Number::New(data_out[4]));
  obj->Set(String::NewSymbol("yaw"), Number::New(data_out[5]));
  return scope.Close(obj);
}

void init(Handle<Object> exports) {
  initMPU();

  if (pthread_create(&readThread, NULL, readFromFIFO, &data_out)) {
    fprintf(stderr, "Error creating thread\n");
    return;
  }
  
  exports->Set(String::NewSymbol("getAttitude"),
    FunctionTemplate::New(GetAttitude)->GetFunction());
  exports->Set(String::NewSymbol("getRotation"),
    FunctionTemplate::New(GetRotation)->GetFunction());
}

void initMPU() {
  // initialize device
  printf("Initializing I2C devices...\n");
  mpu.initialize();

  // verify connection
  printf("Testing device connections...\n");
  printf(mpu.testConnection() ? "MPU6050 connection successful\n" : "MPU6050 connection failed\n");

  printf("Initializing DMP...\n");
  devStatus = mpu.dmpInitialize();

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // turn on the DMP, now that it's ready
    printf("Enabling DMP...\n");
    mpu.setDMPEnabled(true);

    // enable Arduino interrupt detection
    //Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
    //attachInterrupt(0, dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    printf("DMP ready!\n");
    dmpReady = true;
 
    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    printf("DMP Initialization failed (code %d)\n", devStatus);
  }

}

void *readFromFIFO(void *ypr_void_ptr) {
  float *ypr_ptr = (float *)ypr_void_ptr;
  float rotation[3] = {0};
  float last_ypr[3] = {0};

  high_resolution_clock::time_point last_read = high_resolution_clock::now();
  high_resolution_clock::time_point current_read; 
 
  for (;;) {
    // if programming failed, don't try to do anything
    if (!dmpReady) return NULL;

    // get current FIFO count
    fifoCount = mpu.getFIFOCount();

    if (fifoCount == 1024) {
        // reset so we can continue cleanly
        mpu.resetFIFO();
        printf("FIFO overflow!\n");

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
    } else if (fifoCount >= 42) {
        // read a packet from FIFO
        mpu.getFIFOBytes(fifoBuffer, packetSize);
	
        // display Euler angles in degrees
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        //printf("ypr  %7.2f %7.2f %7.2f\n", ypr[0] * 180/M_PI, ypr[1] * 180/M_PI, ypr[2] * 180/M_PI);
	ypr_ptr[0] = ypr[0] * 180/M_PI;
	ypr_ptr[1] = ypr[1] * 180/M_PI;
	ypr_ptr[2] = ypr[2] * 180/M_PI;

	// calcuate rotation
	current_read = high_resolution_clock::now();
	duration<double> delta = duration_cast<duration<double>>(current_read - last_read);
	
	//ypr_ptr[3] = (float)delta.count();
	
	rotation[0] = (ypr_ptr[0] - last_ypr[0]) / (float)delta.count();	
	rotation[1] = (ypr_ptr[1] - last_ypr[1]) / (float)delta.count();
	rotation[2] = (ypr_ptr[2] - last_ypr[2]) / (float)delta.count();

	ypr_ptr[3] = rotation[0];
	ypr_ptr[4] = rotation[1];
	ypr_ptr[5] = rotation[2];

	last_read = current_read;
	last_ypr[0] = ypr_ptr[0];
	last_ypr[1] = ypr_ptr[1];
	last_ypr[2] = ypr_ptr[2];
    }
  }
  return NULL;
}

NODE_MODULE(binding, init);
