# cpds-agent
<p align="center">
<a href="https://gitee.com/openeuler/Cpds"><img src="docs/images/cpds-icon.png" alt="banner" width="250px"></a>
</p>
#### Introduce
Collect Container info for Container Problem Detect System.
Cpds-agent indicates CPDS(Container Problem Detect System)Information acquisition component of container fault detection system

This component collects data required by cpds-detetor(exception detection component).
#### Compiling from source
`cpds-detector`only supported Linux。
```bash
cd $GOPATH/gitee.com/cpds
git clone https://gitee.com/openeuler/cpds-agent.git
cd cpds-agent

make
```
generate after compiling`cpds-agent`
#### The installation
make execute after compiling make install command installation
```
   make install
   systemctl status cpds-agent 
```
#### Uninstall
execute make uninstall uninstalling an installation cpds-agent the environment
```
    make uninstall
```
#### Participate in contribution
1.  Fork this warehouse
2.  New Feat_xxx branch
3.  Submit code
4.  New Pull Request
