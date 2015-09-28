{
	"targets": [
		{
			"target_name": "binding",
			"sources": [ "src/binding.cc", "src/I2Cdev.cpp", "src/MPU6050.cpp" ],
			"defines": ["DMP_FIFO_RATE=9"],
			"cflags": [ "-std=c++11" ],
			"include_dirs" : [
				"<!(node -e \"require('nan')\")"
			]
		}
	]
}
