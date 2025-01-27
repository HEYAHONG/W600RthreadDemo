# 说明

这是一个使用[rt-thread](https://www.rt-thread.org/)开发W60x的例子(仅供测试)。

注意:如需使用Flash升级区,请使用2M芯片。

## 源代码下载

**注意:由于换行符问题,请在Rt-Thread Env中使用git下载或者下载后在下载目录执行git reset --hard。**

由于本源代码包含第三方源代码,故直接下载可能有部分源代码缺失，需要通过以下方法解决:

- 在进行git clone 使用--recurse-submodules参数。

- 若已通过git clone下载,则在源代码目录中执行以下命令下载子模块:

  ```bash
   git submodule update --init --recursive
  ```

## 脚本说明

所有脚本均需要在Rt-Thread Env中执行。

- bootstrap.bat:工程初始化脚本。若工程进行了修改，请先使用git保存变更后再执行此脚本以防止修改丢失。
- menuconfig.bat:Kconfig配置及组件裁剪脚本,必须在至少执行一次（自源代码成功下载）bootstrap.bat后使用。
- codestyle.bat:代码整理脚本,整理src目录下的源代码。每次提交前推荐使用此脚本整理代码。

## 资源文件

类似于桌面程序的资源文件。源代码实现的目录为 [rc](rc/)。

在固件编写中，很多时候需要大量的固定数据，直接手工嵌入到C文件里比较麻烦。

通过读取文件转换到对应C文件可大大节省时间，可添加常用的文件（如各种证书）或者不适宜放在可读写的文件系统中的文件(如需要在格式化中保留或者初始参数)。转换程序源代码为[rc/fsgen.cpp](rc/fsgen.cpp)。

使用步骤如下:

- 将待添加的文件放入 rc/fs目录下。

- 使用文件名调用以下函数(需包含相应头文件RC.h):

  ```c++
  //通过名称获取资源大小
  size_t RCGetSize(const char * name);
  
  //通过名称获取资源指针
  const unsigned char * RCGetHandle(const char * name);
  ```

## OneShot ApWeb文件目录

参考官方SDK,使用Keil默认的工具链的特性,覆盖了默认的web服务器文件系统。可定制在OneShot模式下的html界面,只需要替换 [rc/fs/oneshot](rc/fs/oneshot) 下的文件并重新执行bootstrap.bat即可。

## 测试截图

![W600RtthreadDemo-1](doc/image/W600RtthreadDemo-1.png)

# 编译条件

- 安装好Rt-Thread Env工具。
- 安装好Keil5环境。
- 安装好设备支持包（非官方版,仅用于添加Flash编程算法）。[doc/W600_DFP.pack](./doc/W600_DFP.pack)
- 操作系统为Windows 7以上

# 编译

- 打开Rt-Thread Env工具(运行env.exe,最好使用管理员权限),切换至工程目录(即bootstrap.bat所在目录)。
- 运行bootstrap.bat,将产生project.uvprojx。
- 使用keil5打开project.uvprojx编译调试。

### 注意

- 添加源代码需要放在src目录中,每次更新了src目录都需要重新运行bootstrap.bat或者menuconfig.bat,在keil5添加文件只是临时的。
- 如果遇到错误,请尝试关闭Rt-Thread Env窗口后重新打开一个新的Rt-Thread Env窗口执行脚本。



# 调试

可采用jlink直接调试。

注意:若出现Jlink弹窗不支持芯片，要求选择芯片，请选择cortex-m3。这主要是Jlink的数据库不能识别该芯片,不影响调试。由于W60X内部启动机制，不支持在调试中复位，需要重新打开调试。

## 串口

UART0 :115200 8N1

# 可烧录的固件

成品固件在rt-thread/bsp/w60x/Bin目录。

可直接使用官方的wm_tool.exe烧录。也可使用 [WMToolHelper](https://github.com/HEYAHONG/WMToolHelper.git) 进行烧录调试。