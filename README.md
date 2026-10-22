# STM32C542CC_IIS3DWB10IS


# Overview
- **Name**: IIS3DWB10IS-V1.0
- **MCU**: STM32C542CCT6
- **IDE**: STM32CUBEMX2+STM32CUBEIDE


# Buy Link
[https://shop192352884.taobao.com/](https://shop192352884.taobao.com/)


# Contact Information

- **Name**: Billy
- **交流群**: 925643491
- **Email**: a845656974@outlook.com
- **Phone**: +86 15622736378
- **CSDN Blog**: [Blog](https://blog.csdn.net/qq_24312945)
- **Video**: [Video](https://space.bilibili.com/26152390)



# Project Introduction
- **STM32C5_IIS3DWB10IS_Project1**:STM32C5开发IIS3DWB10IS(1)----SPI获取震动计数据
- **CSDN Blog**:[https://blog.csdn.net/qq_24312945/article/details/163568700](https://blog.csdn.net/qq_24312945/article/details/163568700)

IIS3DWB10IS采用系统级封装，配备3轴数字振动传感器 ，在超宽平坦频率范围内保持低噪声。这款器件具有宽带宽、低噪声、高稳定性和可重复的灵敏度，能在扩展温度范围（最高+125°C）下工作，且量程加速度可达±200 g，因此特别适合各种工业应用（包括在最恶劣和要求最严苛的环境）中的振动监测。
IIS3DWB10IS具有可选的±50/±100/±200 g量程加速度范围，能够测量带宽超过10 kHz的加速度。该器件集成了一个2048槽位的先进先出 (FIFO ) 缓冲区，用于批量处理来自集成加速度计、温度传感器或ISPU的数据，同时避免数据丢失并减少主处理器的干预。
IIS3DWB10IS内嵌意法半导体的一款新型处理器件ISPU（智能传感器处理单元），支持依赖传感器数据的实时应用。ISPU是一个超低功耗、高性能的可编程内核，能够执行实时信号处理和边缘AI算法 。ISPU支持C语言编程，并可依托意法半导体及第三方提供的库、工具/IDE构成的生态系统。
其针对算法实时执行而优化的超低功耗硬件电路是面向工业5.0的任何无线传感器节点的一项先进功能。
意法半导体的MEMS传感器模块系列采用稳健成熟的制造工艺，已经在微机械加工的加速度计和陀螺仪产品中广泛应用，可满足汽车、工业和消费类市场的需求。传感元件采用意法半导体专门的微型机械加工工艺制造，而内嵌的IC接口采用CMOS技术开发。
IIS3DWB10IS具有自检功能，可用于检查传感器在最终应用中的功能。IIS3DWB10IS采用16引脚塑料焊盘栅格阵列 (LGA) 封装，可确保在扩展温度范围（-40 °C至+125 °C）内正常工作。

The IIS3DWB10IS is a system-in-package featuring a 3-axis digital vibration sensor that delivers low noise over an ultrawide, flat frequency response range. With its wide bandwidth, low noise, high stability, repeatable sensitivity, operation over an extended temperature range up to +125°C, and acceleration measurement ranges up to ±200 g, the device is particularly well suited to vibration monitoring in a wide variety of industrial applications, including those operating in the harshest and most demanding environments.

The IIS3DWB10IS offers selectable full-scale acceleration ranges of ±50/±100/±200 g and can measure acceleration over a bandwidth exceeding 10 kHz. The device integrates a 2048-slot first-in, first-out (FIFO) buffer for batching data from the integrated accelerometer, temperature sensor, or ISPU, preventing data loss while reducing intervention by the host processor.

The IIS3DWB10IS incorporates STMicroelectronics’ innovative ISPU (Intelligent Sensor Processing Unit), which supports real-time applications that rely on sensor data. The ISPU is an ultralow-power, high-performance programmable core capable of executing real-time signal processing and edge AI algorithms. It supports programming in C and benefits from an ecosystem of libraries, tools, and integrated development environments (IDEs) provided by STMicroelectronics and third parties.

Its ultralow-power hardware, optimized for real-time algorithm execution, provides an advanced capability for wireless sensor nodes targeting Industry 5.0 applications.

STMicroelectronics’ MEMS sensor module family uses robust, mature manufacturing processes that have been widely deployed in micromachined accelerometers and gyroscopes for the automotive, industrial, and consumer markets. The sensing element is manufactured using STMicroelectronics’ proprietary micromachining process, while the embedded interface IC is developed using CMOS technology.

The IIS3DWB10IS includes a self-test function that allows users to verify sensor functionality in the final application. It is available in a 16-lead plastic land grid array (LGA) package and is guaranteed to operate over an extended temperature range of −40°C to +125°C.

- **STM32C5_IIS3DWB10IS_Project2**:STM32C5开发IIS3DWB10IS(2)----IIC获取震动计数据
- **CSDN Blog**:[https://blog.csdn.net/qq_24312945/article/details/166256754](https://blog.csdn.net/qq_24312945/article/details/166256754)

本章介绍如何使用 STM32C542CCT6 通过 IIC 接口驱动 IIS3DWB10IS，实现三轴加速度和温度数据的轮询读取。内容包括硬件连接、CubeMX 外设配置、驱动接口移植以及传感器初始化，并完成量程和输出数据率设置。采集到的原始数据经过单位换算后，通过 USART1 输出到串口 调试助手，便于观察数据变化，为后续振动监测应用开发提供基础。

This chapter describes how to interface the STM32C542CCT6 with the IIS3DWB10IS via I²C to read three-axis acceleration and temperature data using polling. It covers hardware connections, peripheral configuration in STM32CubeMX, porting the driver interface, and sensor initialization, including full-scale range and output data rate settings. The raw measurements are converted into physical units and transmitted via USART1 to a serial terminal for monitoring, providing a foundation for further development of vibration monitoring applications.

# Image


<img width="1280" height="1810" alt="image" src="https://github.com/user-attachments/assets/8e5ae48d-d008-4f7e-b5c1-43dd1d90ca29" />


<img width="1285" height="1809" alt="image" src="https://github.com/user-attachments/assets/e6e5ddf8-0794-4699-9b40-eb50e21a72bb" />

<img width="1281" height="1814" alt="image" src="https://github.com/user-attachments/assets/4a91bcdf-cc8e-42cf-8578-29825a1dcae9" />

<img width="1285" height="1814" alt="image" src="https://github.com/user-attachments/assets/dee67d2b-b966-4048-a36a-9a81f9dead98" />


<img width="1284" height="1814" alt="image" src="https://github.com/user-attachments/assets/8eba924e-a862-4ec4-a5fe-b482a8175f8a" />


<img width="1280" height="1813" alt="image" src="https://github.com/user-attachments/assets/16458048-9415-4a9c-916a-57af4a1f1afa" />















