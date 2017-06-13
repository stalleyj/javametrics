To build native agent

** On Windows **

Make sure you are running inside a visual studio command shell to pick up the compiler.  There are 2 versions, a 32 and a 64 bit one so make sure you are running the right one for the level you want to build

*****************

`cd native`

`make BUILD=wa64 clean javametrics`  (to build on 64 bit windows)

** On Mac **

You need to include two directories in the JAVA_SDK_INCLUDE path, e.g.:
export "JAVA_SDK_INCLUDE=/Library/Java/JavaVirtualMachines/jdk1.8.0_121.jdk/Contents/Home/include/ -I/Library/Java/JavaVirtualMachines/jdk1.8.0_121.jdk/Contents/Home/include/darwin/"

`make BUILD=darwin64 clean javametrics`
