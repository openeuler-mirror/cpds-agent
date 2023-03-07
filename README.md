# cpds-agent

<div align=center>
<img src="docs/images/cpds-icon.png" width="250px"/>
</div>

#### 介绍
cpds-agent是为CPDS(Container Problem Detect System)容器故障检测系统开发的信息采集组件

本组件根据cpds-detetor(异常检测组件)需要的数据进行相应采集。

#### 从源码编译
`cpds-detector`只支持Linux。

编译依赖：
* cmake: 版本不低于3.14.5
* libmicrohttpd
* glib-2.0

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
