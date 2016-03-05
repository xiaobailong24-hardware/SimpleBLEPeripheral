# SimpleBLEPeripheral
SimpleBLEPeripheral--CC2540F256 Connects Android Phone BLE

##开发软件：IAR 9.30.1

##蓝牙协议栈：BLE-CC254x-1.4.1.43908b

##硬件：WEBEE蓝牙开发板(CC2540F256)

##Android APP：BLE Scanner(Android4.3+)

##实现的功能：

	1.设置蓝牙模块为从机(Peripheral),开机时广播，LED1闪烁，可被其他BLE主机发现并连接。初始密码：000000
	2.Android手机安装BLE Scanner,可对蓝牙模块进行扫描并连接，连接后读取蓝牙模块的Profile，此时LED1灭，LED3亮。
	3.利用BLE Scanner对蓝牙模块得特征值进行读写，可控制其LED2灯的亮灭。
	
##在线视频
	Youtube：<iframe width="420" height="315" src="https://www.youtube.com/embed/Rus9GKjR81k" frameborder="0" allowfullscreen></iframe>
