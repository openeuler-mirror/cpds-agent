# cpds-agent

<div align=center>
<img src="docs/images/cpds-icon.png" width="250px"/>
</div>

#### 介绍
cpds-agent是为CPDS(Container Problem Detect System)容器故障检测系统开发的信息采集组件

本组件根据cpds-detetor(异常检测组件)需要的数据进行相应采集。

#### 从源码编译
`cpds-agent`只支持Linux。

编译依赖：
* cmake: 版本不低于3.14.5
* glib-2.0
* libmicrohttpd
* libsystemd
* clang: 版本不低于10.0
* bpftool
* elfutils
* libcurl
* 内核版本 5.10 及以上
   * 需要开启内核选项 CONFIG_DEBUG_INFO_BTF=y

对于`openEuler`系统(22.03 LTS 及其以上)，安装命令如下：
``` shell
dnf install -y cmake
dnf install -y glib2-devel
dnf install -y libmicrohttpd-devel
dnf install -y systemd-devel
dnf install -y clang
dnf install -y bpftools
dnf install -y elfutils-devel
dnf install -y libcurl-devel.aarch64
```

下载cpds-agent并编译：
```
git clone https://gitee.com/openeuler/cpds-agent.git
cd cpds-agent
./build.sh
```

#### 安装
```
./build.sh install
```

#### 卸载
```
./build.sh uninstall
```

#### 参与贡献
1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request
