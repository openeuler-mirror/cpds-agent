# cpds-agent

#### 介绍
Collect Container info for Container Problem Detect System.
cpds-agent是为CPDS(Container Problem Detect System)容器故障检测系统开发的信息采集组件

本组件根据cpds-detetor(异常检测组件)需要的数据进行相应采集。
#### 从源码编译
`cpds-detector`只支持Linux。
```bash
cd $GOPATH/gitee.com/cpds
git clone https://gitee.com/openeuler/cpds-agent.git
cd cpds-agent

make
```
编译完成后生成`cpds-agent`
#### 安装
make编译过后执行make install命令安装
```
   make install
   systemctl status cpds-agent查看服务状态
```
#### 卸载
执行make uninstall卸载已安装cpds-agent的环境
#### 参与贡献
1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


