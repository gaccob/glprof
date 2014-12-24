glprof 是什么
-------------
glprof 是一个简单的 Lua profier 工具, C++ 实现, 并有简单的 Lua 导出接口.
- start(), profiler 工具开始记录;
- stop(), profiler 工具停止记录;
- cleanup(), 清除 profiler 统计数据;
- dump_file(), dump profiler 数据到文件;
- dump_string(), 返回一个 profiler 统计数据的字符串;


glprof 的实现
-------------
glprof 通过 Lua 的 hook 机制实现, 它可以记录每次函数(包括 Lua 函数和调用的 C 函数)的时间片开销, 以及调用次数, 最终输出到文本, 方便做性能分析.


glprof 的参考
-------------
绝大部分的参考都来自 OWenT 同学这份 blog：[《Lua性能分析》](https://github.com/owt5008137/OWenT.Articles/blob/master/Markdown/Lua%E6%80%A7%E8%83%BD%E5%), 包括参考了 OWenT 同学的源码实现, 特别感谢. 因为不太习惯 C++ 11 和大量的模版实现, 所以花了点时间重新写了一个更适合目前项目的版本. 


从来没有 do 过的 todo
---------------------
目前有两个小瑕疵, 因为暂时不影响项目中的使用, 所以暂时不打算解决了:
- 尾部调用 (tail call) 通过 hook 接口拿不到函数名, 所以只能以 file:line 来代替标识;
- 递归 (包括 tail call) 的统计会不准确, 因为的 tail call 实际上只是在记录的栈帧上打了个标签;
