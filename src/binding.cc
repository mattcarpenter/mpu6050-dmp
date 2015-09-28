#include "binding.h"

using v8::FunctionTemplate;
using namespace std::chrono;

NAN_METHOD(GetAttitude) {
  v8::Local<v8::Object> obj = Nan::New<v8::Object>();
  Nan::Set(obj, Nan::New("roll").ToLocalChecked(), Nan::New(data_out[1]));
  Nan::Set(obj, Nan::New("pitch").ToLocalChecked(), Nan::New(data_out[2]));
  Nan::Set(obj, Nan::New("yaw").ToLocalChecked(), Nan::New(data_out[0]));
  info.GetReturnValue().Set(obj);
}

NAN_METHOD(GetRotation) {
  v8::Local<v8::Object> obj = Nan::New<v8::Object>();
  Nan::Set(obj, Nan::New("roll").ToLocalChecked(), Nan::New(data_out[3]));
  Nan::Set(obj, Nan::New("pitch").ToLocalChecked(), Nan::New(data_out[4]));
  Nan::Set(obj, Nan::New("yaw").ToLocalChecked(), Nan::New(data_out[5]));
  info.GetReturnValue().Set(obj);
}

NAN_METHOD(Initialize) {
  if (!initMPU()) {
    info.GetReturnValue().Set(false);
    return;
  }

  if (pthread_create(&readThread, NULL, readFromFIFO, &data_out)) {
    fprintf(stderr, "Error creating thread\n");
    info.GetReturnValue().Set(false);
  } else {
    info.GetReturnValue().Set(true);
  }
}

bool initMPU() {
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

    return true;
  } else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    printf("DMP Initialization failed (code %d)\n", devStatus);
    return false;
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

NAN_MODULE_INIT(InitAll) {
  Nan::Set(target, Nan::New("getAttitude").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(GetAttitude)).ToLocalChecked());
  Nan::Set(target, Nan::New("getRotation").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(GetRotation)).ToLocalChecked());
  Nan::Set(target, Nan::New("initialize").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(Initialize)).ToLocalChecked());
}

NODE_MODULE(binding, InitAll)