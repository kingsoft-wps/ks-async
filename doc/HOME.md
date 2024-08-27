# `ks-async异步框架 API文档 （HOME）`

# 目录

#### 关于Future：
- [ks_future\<T>](ks_future.md)：ks_future对象
- [ks_promise\<T>](ks_promise.md)：ks_promise对象
<br><br>

#### 与Future相关：
- [ks_apartment](ks_apartment.md)：异步过程执行套间
- [ks_async_context](ks_async_context.md)：异步过程上下文
- [ks_async_controller](ks_async_controller.md)：异步过程控制器
- [ks_result\<T>](ks_result.md)：结果对象
- [ks_error](ks_error.md)：错误值
<br><br>

#### 高级异步任务抽象（进化）：
- [ks_async_task\<T, ARG...>](ks_async_task.md)：可延迟编排的异步任务高级封装
<br><br>

<br>
<br>



# ks_future异步框架介绍

在很多语言/框架中都可以看到future这个词。但在不同的世界里future存在显著的差异，大体上存在两类Future模型：C++和Java提供的模型、JS和Scala提供的模型。

C++和Java提供的模型太弱了，而JS和Scala提供的是比较理想的Future模型。我们的ks_future异步框架就是据此进行设计的，并进行了一些调整和增强。

这里不对Future到底是个什么进行赘述，请自行查阅JS和Scala等资料。这里重点讲我们的ks_future框架提供一些特殊设计和能力。

<br>


## ks_future\<T>、ks_promise\<T>

ks_future和ks_promise没啥可说的。

<br>


## ks_result\<T>、ks_error

一个ks_result对象用来表示一个异步结果（比如ks_future的最终完成结果），它或者是未完成的、或者表示一个T类型值（代表成功）、或者表示一个ks_error（代表失败）。在ks_future框架中，“错误” 统一用ks_error表示。

ks_result还具有不可变性，其内包含的 “值” 是不可变的。然而，ks_result变量（非const）本身仍是可以被赋值的，赋值后其内将包含一个全新的“值实例”，而非覆写旧“值实例”。

<br>


## ks_apartment（套间）

实际上就是用来执行异步过程的线程（池）抽象，大体上与scala的ExecutionContext对应。（但我们未以context命名套间，且我们的context另有所指，参见ks_async_context）

我们的ks_future异步框架特殊之处在于，所有涉及异步过程的方法都会明确要求调用者传入一个apartment参数，以明确指出期望异步过程终将在哪个套间中执行。

实际上，框架明确了4个标准的套间：ui_sta（UI[单线程]套间 master_sta（主逻辑[单线程]套间）、background_sta（后台[单线程]套间）、default_mta（默认[多线程]套间）。

其中，ui_sta和master_sta需要由APP框架提供，而background_sta和default_mta可以直接使用现成的标准实现。

有赖于此，开发者就可以明确各异步过程中是否需要、以及该如何做多线程安全保护。

<br>


## ks_async_context（异步过程上下文）、ks_async_controller（异步过程控制器）

与Java、JS、scala等语言不同，C++中没有GC概念，所有对象都需要手工进行生命期管理，而这一点在异步模型中会是一个巨大的挑战。

C++中自动化对象生命期管理的手段无非两种：智能指针（如ks_stdptr和shared_ptr）、对象树（如QObject）。

context和control就是为辅助解决对象生命期而设计的。context可以维持一组智能指针，control可以中断异步过程（或等待结束）。context构造时要求传入一个owner对象智能指针（可选）和一个control指针（可选）。

所有涉及异步过程的方法都会明确要求调用中传入一个context参数，该context内所持对象在相应异步过程执行完毕前保持有效。

<br>

<br>
<br>




# 外部参考资料

Scala Future: https://www.scala-lang.org/api/current/scala/concurrent/Future.html

JS Promise: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise

