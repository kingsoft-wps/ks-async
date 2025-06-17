# `namespace ks_future_util`

# 说明

ks_future_util工具集

<br>
<br>
<br>



```C++
ks_future<T> ks_future_util::resolved<T>(const T& value);
ks_future<T> ks_future_util::rejected<T>(const ks_error& error);

ks_future<T> ks_future_util::post<T>(ks_apartment* apartment, function<T()> task_fn, const ks_async_context& context = {});
ks_future<T> ks_future_util::post<T>(ks_apartment* apartment, function<ks_result<T>()> task_fn, const ks_async_context& context = {});
ks_future<T> ks_future_util::post<T>(ks_apartment* apartment, function<ks_result<T>(ks_cancel_inspector*)> task_fn, const ks_async_context& context = {});

ks_future<T> ks_future_util::post_delayed<T>(ks_apartment* apartment, function<T()> task_fn, int64_t delay, const ks_async_context& context = {});
ks_future<T> ks_future_util::post_delayed<T>(ks_apartment* apartment, function<ks_result<T>()> task_fn, int64_t delay, const ks_async_context& context = {});
ks_future<T> ks_future_util::post_delayed<T>(ks_apartment* apartment, function<ks_result<T>(ks_cancel_inspector*)> task_fn, int64_t delay, const ks_async_context& context = {});

ks_future<T> ks_future_util::post_pending<T>(ks_apartment* apartment, function<T()> task_fn, ks_pending_trigger* trigger, const ks_async_context& context = {});
ks_future<T> ks_future_util::post_pending<T>(ks_apartment* apartment, function<ks_result<T>()> task_fn, ks_pending_trigger* trigger, const ks_async_context& context = {});
ks_future<T> ks_future_util::post_pending<T>(ks_apartment* apartment, function<ks_result<T>(ks_cancel_inspector*)> task_fn, ks_pending_trigger* trigger, const ks_async_context& context = {});
```
#### 描述：包装ks_future<T>的resolved和post系列方法，直接以模板函数的形式提供能力。
#### 特别说明：以resolved方法为例，`ks_future<int>::resolved(1)` 的等价形式为：`ks_future_util::resolved<int>(1)`。
<br>
<br>


```C++
ks_future<vector<T>> ks_future_util::all(const vector<ks_future<T>>& futures);
ks_future<tuple<T0, T1, ...>> ks_future_util::all(const ks_future<T0>& future0, const ks_future<T1>& future1, ...);
```
#### 描述：创建一个ks_future对象，代表所有指定futures全部 “成功”。
#### 参数：
  - futures: 前序ks_future对象数组。
  - future0, future1, ...: 前序ks_future对象列表。
#### 返回值：新ks_future对象，其 “值” 类型为tuple\<T0, T1, ...>。若有前序future失败，则转发该 “错误”。
#### 特别说明：若某前序future失败，则立即处置此聚合ks_future为失败，不会等待其他future完成（然而会自动尝试tryTancel它们，但并无保证）。
<br>

```C++
ks_future<T> ks_future_util::any(const vector<ks_future<T>>& futures);
ks_future<T> ks_future_util::any(const ks_future<T>& future0, const ks_future<T>& future1, ...);
```
#### 描述：创建一个ks_future对象，代表任一（实际上就是最先）future “成功”。
#### 参数：
  - futures: 前序ks_future对象数组。
  - future0, future1, ...: 前序ks_future对象列表。
#### 返回值：新ks_future对象，其 “值” 类型为R。若全部前序future失败，则转发首个 “错误”。
#### 特别说明：若某前序future成功，则立即处置此聚合ks_future为成功，不会等待其他future完成（然而会自动尝试try_cancel它们，但并无保证）。
<br>
<br>


```C++
ks_future<void> parallel(
		ks_apartment* apartment, 
    const vector<function<void()>>& fns, 
		const ks_async_context& context = {});
ks_future<void> parallel_n(
		ks_apartment* apartment, 
    function<void()> fn, size_t n,
		const ks_async_context& context = {});
```
#### 描述：并行执行多个异步过程。
#### 参数：
  - apartment: 异步执行套间。
  - fns: 异步函数序列。
  - fn, n: 异步函数，个数。
  - context: 异步任务执行时所需上下文。
#### 返回值：代表迭代结束的一个future。
<br>

```C++
ks_future<void> sequential(
		ks_apartment* apartment, 
    const vector<function<void()>> fns, 
		const ks_async_context& context = {});
ks_future<void> sequential_n(
		ks_apartment* apartment, 
    function<void()> fn, size_t n,
		const ks_async_context& context = {});
```
#### 描述：串行执行多个异步过程。
#### 参数：
  - apartment: 异步执行套间。
  - fns: 异步函数序列。
  - fn, n: 异步函数，个数。
  - context: 异步任务执行时所需上下文。
#### 返回值：代表迭代结束的一个future。
<br>
<br>


```C++
ks_future<void> repeat(
		ks_apartment* apartment, 
    function<ks_result<void>()> fn, 
		const ks_async_context& context = {});
```
#### 描述：重复执行一个异步过程，直至EOF或错误。
#### 参数：
  - apartment: 异步执行套间。
  - fn: 异步函数。
  - context: 异步任务执行时所需上下文。
#### 返回值：代表迭代结束的一个future，若至EOF结束则返回成功。
<br>

```C++
ks_future<void> repeat_periodic(
		ks_apartment* apartment, 
    function<ks_result<void>()> fn, 
    int64_t first_delay, int64_t interval, 
		const ks_async_context& context = {});
```
#### 描述：定时重复执行一个异步过程，直至EOF或错误。
#### 参数：
  - apartment: 异步执行套间。
  - fn: 异步函数。
  - first_delay: 首次执行时延。
  - interval: 每次执行间隔时间。
  - context: 异步任务执行时所需上下文。
#### 返回值：代表迭代结束的一个future，若至EOF结束则返回成功。
<br>

```C++
template <class V>
ks_future<void> ks_future_util::repeat_repetitive<V>(
    ks_apartment* produce_apartment, function<ks_result<V>()> produce_fn,
    ks_apartment* consume_apartment, function<ks_result<void>(const U&)> consume_fn,
    const ks_async_context& context = {});
```
#### 描述：反复迭代一个异步的produce-consume过程，直至EOF或错误。
#### 模板参数：
  -- V: produce_fn每次产生的、以及consume_fn消费的数据类型。
#### 参数：
  - produce_apartment: 生产者执行套间。
  - produce_fn: 生产者异步函数。
  - consume_apartment: 消费者执行套间。
  - consume_fn: 消费者异步函数。
  - context: 异步任务执行时所需上下文。
#### 返回值：代表迭代结束的一个future，若至EOF结束则返回成功。
<br>
<br>
<br>
<br>


# 另请参阅
  - [HOME](HOME.md)
  - [ks_promise\<T>](ks_promise.md)
  - [ks_result\<T>](ks_result.md)
