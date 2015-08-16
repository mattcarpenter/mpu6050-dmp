var mpu = require('./../build/Debug/binding');

if (mpu.initialize()) {
  setInterval(function () {
    console.log(mpu.getRotation());
    console.log(mpu.getAttitude());
  }, 10);
}
