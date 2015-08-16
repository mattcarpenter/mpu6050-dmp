# MPU6050-dmp

Quick and dirty Node.js module for reading DMP-processed yaw/pitch/roll and rotation data from an MPU6050.

This module has been developed and tested on a BeagleBone Black but will likely work on a Raspberry Pi as well with a slight modification (detailed below).

## Install

```
npm install mpu6050-mpu
```

## Usage

```javascript
var mpu = require('mpu6050-mpu');

mpu.initialize();

setTimeout(function () {
    console.log(mpu.getAttitude());
    console.log(mpu.getRotation());
}, 5);
```

## Credits and License

The DMP initialization routine borrows code from Richard Hirst's [MPU6050-Pi-Demo](https://github.com/richardghirst/PiBits/tree/master/MPU6050-Pi-Demo), which ports a ton of functionality to Raspberry Pi from Jeff Rowberg's [i2cdevlib](https://github.com/jrowberg/i2cdevlib). 

This project is licensed under the terms of the MIT license.
