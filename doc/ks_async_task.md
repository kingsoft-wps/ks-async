# `template <Class T, class ARG...>` <br> `class ks_async_task<T, ARG...>`

# 说明

可延迟编排的异步任务高级封装。

<br>
<br>



# 构造方法

```C++
ks_async_task<T>(const T& value);
ks_async_task<T>(const ks_future<T>& future);

ks_async_task<T>(ks_apartment* apartment, const ks_async_context& context, function<T()>&& fn);
ks_async_task<T>(ks_apartment* apartment, const ks_async_context& context, function<ks_result<T>()>&& fn);

ks_async_task<T>(ks_apartment* apartment, const ks_async_context& context, function<T()>&& fn, ks_pending_trigger* trigger);
ks_async_task<T>(ks_apartment* apartment, const ks_async_context& context, function<ks_result<T>()>&& fn, ks_pending_trigger* trigger);

ks_async_task<T, ARG>(ks_apartment* apartment, const ks_async_context& context, function<T(const ARG&)>&& fn);
ks_async_task<T, ARG>(ks_apartment* apartment, const ks_async_context& context, function<ks_result<T>(const ARG&)>&& fn);

ks_async_task<T, ARG0, ARG1, ...>(ks_apartment* apartment, const ks_async_context& context, function<T(const ARG0&, const ARG1&, ...)>&& fn);
ks_async_task<T, ARG0, ARG1, ...>(ks_apartment* apartment, const ks_async_context& context, function<ks_result<T>(const ARG0&, const ARG1&, ...)>&& fn);
```

#### 描述：创建ks_async_task对象。若有ARG...模板参数，则通常还需要调用connect与上游异步任务进行关联。

<br>
<br>


# 任务编排方法

```C++
void ks_async_task<T, ARG>::connect(const ks_async_task<ARG, ？>& prev_task);
void ks_async_task<T, ARG>::connect(const ks_future<ARG>& prev_future);

void ks_async_task<T, ARG0, ARG1, ...>::connect(const ks_async_task<ARG0, ？>& prev_task0, const ks_async_task<ARG1, ？>& prev_task1, ...);
void ks_async_task<T, ARG0, ARG1, ...>::connect(const ks_future<ARG0>& prev_future0, const ks_future<ARG1>& prev_future1, ...);
```

#### 描述：异步任务编排。将this任务接续在指定prev任务的后面，prev是上游、this成为下游。一个对象只允许执行一次connect调用。

<br>
<br>


# 一般成员方法

```C++
ks_future<T> get_future() const;
```

#### 描述：获取结果future。

<br>
<br>
<br>
<br>




# 另请参阅
  - [HOME](HOME.md)
  - [ks_future\<T>](ks_future.md)
  - [ks_promise\<T>](ks_promise.md)
