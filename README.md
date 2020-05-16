# PatXmlParser

从主菜单中选择“调试”>“调试和启动${activeDebugTarget}的设置” ，以自定义特定于活动调试目标的调试配置。
输入以下：

```
{
  "version": "0.2.1",
  "defaults": {},
  "configurations": [
    {
      "type": "default",
      "project": "CMakeLists.txt",
      "projectTarget": "src.exe",
      "name": "src.exe",
      "args": [ "config.ini", "listtest.txt", "outpath", "10", "lawstate.txt" ]
    },
    {
      "type": "default",
      "project": "CMakeLists.txt",
      "name": "CMakeLists.txt"
    }
  ]
}
```
