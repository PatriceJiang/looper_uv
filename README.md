

> support win32 only 

## 提供

- 使用`libuv`作为后端
- 提供`Event(on/emit)`事件通讯
- 内置消息队列, 保证调用顺序
- 提供`Loop#update`主循环
- 方便调度计算体到不同的线程, 减少锁在多数情形的使用
