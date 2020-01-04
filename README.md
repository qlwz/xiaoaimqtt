# 小爱MQTT、拦截程序

## 介绍 MQTT  
1. 支持当前播放内容上传  主题：xiaoai/序列号/stat/context  JSON数据  
2. 支持当前音量上传      主题：xiaoai/序列号/stat/volume  0-100  
3. 支持当前播放上传      主题：xiaoai/序列号/stat/status  0-2  0：开机未播放 1：播放中 2：暂停播放  
4. 支持下发TTS          主题：xiaoai/序列号/cmnd/tts     播放文字  
5. 支持下发音量大小      主题：xiaoai/序列号/cmnd/volume  0-100|up|down  
6. 支持下发控制播放      主题：xiaoai/序列号/cmnd/player  ch|prev|next|play|pause|toggle|http地址  
7. 支持下发shell命令    主题：xiaoai/序列号/cmnd/cmd      shell命令
  
## 介绍 拦截  
   让小爱支持控制任意设备  
   程序主要逻辑来源于：https://github.com/FlashSoft/mico  
>    主要逻辑是：  
> 1. 检测小米服务器响应的日志变化  
> 2. 捕获响应日志中，如果是未知设备（也等于你自己的自制设备）  
> 3. 则让小爱停止播放找到未知设备的播报  
> 4. 通过curl转发asr和res日志内容给自己的远端接口  
> 5. 远端接口去跟HA通讯来控制自制设备  
> 6. 远端接口返回需要小爱播报的文本内容  
> 7. 如果之前小爱在播放音乐的话就接着播放音乐  

## 如何安装

### 黑版小爱 （文件系统可读写）
1. 现在xiaoaimqtt文件放在 /data下
2. 添加并编辑该文件/etc/init.d/mico_enable    
  
```
    #!/bin/sh /etc/rc.common  
    START=96  
    start() {  
       /data/xiaoaimqtt &  
    }  
    
    stop() {  
      kill `ps|grep '/data/xiaoaimqtt'|grep -v grep|awk '{print \$1}'`  
    }  
```

3. 设置权限在 shell下执行
    chmod a+x /data/xiaoaimqtt  
    chmod a+x /etc/init.d/mico_enable  
    /etc/init.d/mico_enable enable  
    /etc/init.d/mico_enable start  
4. 浏览器打开小爱的IP 设置 接口 MQTT 等信息  

### 绿板小爱 （文件系统不可写）
1  现在xiaoaimqtt文件放在 /data下
2. 设置权限 在shell下执行
    chmod a+x /data/xiaoaimqtt  
3. 使用其他方式来启动，参考以下  

### 单片机玩家一
   参照连接完成破解 https://bbs.hassbian.com/forum.php?mod=viewthread&tid=8903

   需要把  
```
    "test `ps|grep 'sh /data/mico.sh'|grep -v grep|wc -l` -eq 0 && sh /data/mico.sh > /tmp/mico.log 2>&1 &"  
```
   换成  
```
    "test `ps|grep '/data/xiaoaimqtt'|grep -v grep|wc -l` -eq 0 && /data/xiaoaimqtt > /tmp/mico.log 2>&1 &"  
```
### 单片机玩家二
   使用 https://github.com/qlwz/esp 里面的xiaoai固件  
   PCB板：https://github.com/qlwz/esp/tree/master/file/pcb/XiaoAi 
   使用的是EPS-12S
   焊接好后打开ESP地址可以设置小米密码，并且会自动启动本软件

### 修改固件
   参考链接完成破解 https://bbs.hassbian.com/forum.php?mod=viewthread&tid=8754&page=1#pid283801
   最后的 /data/init.sh 文件追加  
```
    /data/xiaoaimqtt &
```


# 如何编译

## Linux
   1. 参照：https://www.cnblogs.com/flyinggod/p/9468612.html 配置环境  
   2. 然后进入目录 make  

## Windows
   1. 从https://blog.csdn.net/lg1259156776/article/details/52281323 里面下载Windows安装版  
   2. 安装 MinGW  
   3. 配置好 make.exe 和 arm-linux-gnueabihf-gcc.exe  
   4. 然后进入目录 make  