本项目用于算法SDK日志的通信中转并回流  

编译环境： 
使用ndk编译，建议版本r17c  
在linux环境下解压缩ndk包之后，在环境变量PATH中加入ndk所在路径  
在windows环境下类似  

编译方法：  
编译so库请在项目根目录下执行ndk-build命令，编译测试用可执行程序请到exe目录执行命令  
在linux环境下具体命令位于build.sh  
在windows环境下具体命令位于build.bat  

关于调试： 
如果需要__android_log_print打印调试信息，需要修改两处：  
a. com/cus_log.h中需要开启宏OPEN_LOG_PRINT  
b. 根目录下的Android.mk中LOCAL_LDLIBS需要开启链接-landroid -llog -ldl -lz（也就是libandroid.so库）  
