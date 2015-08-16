# MPU6050-dmp

Quick and dirty Node.js module for reading DMP-processed yaw/pitch/roll and rotation data from an MPU6050. 

This module has been developed and tested on a BeagleBone Black but should also work on a Raspberry Pi as well.

## Install

```
npm install mpu6050-dmp
```

## Usage

```javascript
var mpu = require('mpu6050-dmp');

if (mpu.initialize()) {
  setInterval(function () {
    console.log(mpu.getRotation());
    console.log(mpu.getAttitude());
  }, 10);
}
```

## Selecting an I2c device

This module is hard-coded to open the i2c device located at `/dev/i2c-1`. To change this, you must simply do a search and replace in `src/I2Cdev.cpp`. I may go back and make this configurable at runtime, but I'd like to expose some of the other MPU and DMP configuration options via the module exports and intialization routine first. 

## Credits and License

The DMP initialization routine borrows code from Richard Hirst's [MPU6050-Pi-Demo](https://github.com/richardghirst/PiBits/tree/master/MPU6050-Pi-Demo), which ports a ton of functionality to Raspberry Pi from Jeff Rowberg's [i2cdevlib](https://github.com/jrowberg/i2cdevlib). 

This project is licensed under the terms of the MIT license.
