# `class ks_apartment`

# 说明

异步过程执行套间。实际上，apartment就是用来执行异步过程的线程（池）抽象。

存在4个标准套间：
  - ui_sta：UI[单线程]套间（一般是主线程，也可以不是）
  - master_sta：主逻辑[单线程]套间（可以与ui-sta相同，但最好区别开）
  - background_sta：后台[单线程]套间
  - default_mta：默认[多线程]套间

所有涉及异步过程的方法都会明确要求调用者传入一个apartment参数，以明确指出期望异步过程终将在哪个套间中执行。

<br>
<br>



# 静态成员方法

```C++
static ks_apartment* ui_sta();
static ks_apartment* master_sta();
static ks_apartment* background_sta();
static ks_apartment* default_mta();
```
#### 描述：获取各种线程套间。
<br>

```C++
static ks_apartment* find_public_apartment(const char* name);
```
#### 描述：获取已注册的公开套间。
<br>

```C++
static ks_apartment* current_thread_apartment();
static ks_apartment* current_thread_apartment_or_default_mta();
static ks_apartment* current_thread_apartment_or(ks_apartment* or_apartment);
```
#### 描述：获取当前线程对应的套间。
#### 返回值：当前线程对应的套间，或nullptr/default_mta/or_apartment。
<br>
<br>


# 一般成员方法

```C++
const char* name();
```
#### 描述：获取套间名称。
#### 返回值：套间名称。
<br>

```C++
const char* features();
```
#### 描述：获取套间特征。
#### 返回值：套间特征。
<br>

```C++
const char* concurrency();
```
#### 描述：获取套间理想并行度值。
#### 返回值：套间理想并行度值。
<br>
<br>


```C++
bool start();
```
#### 描述：开始。
#### 返回值：是否成功。
<br>

```C++
void async_stop();
```
#### 描述：异步停止（不等待）。
<br>

```C++
void wait();
```
#### 描述：等待（要求先async_stop）。
<br>

```C++
bool is_stopped();
bool is_stopping_or_stopped();
```
#### 描述：判定套间是否已停止、或正在被停止。
<br>
<br>


```C++
uint64_t schedule(function<void()> fn, int priority);
```
#### 描述：调度一个异步过程。
#### 参数：
  - fn: 异步过程函数。
  - priority: 异步过程优先级。0：一般优先级，1：高优先级，-1：低优先级。
#### 返回值：返回一个id值，代表该异步过程。若失败则返回0值。
#### 特别说明：通常我们不应直接使用此方法，而是使用ks_future\<T>::post发起异步任务。
<br>

```C++
uint64_t schedule_delayed(function<void()> fn, int priority, int64_t delay);
```
#### 描述：调度一个异步**延时**过程。
#### 参数：
  - fn: 异步过程函数。
  - priority: 异步过程优先级。0：一般优先级，1：高优先级，-1：低优先级。
  - delay：延时时长。单位：毫秒。
#### 返回值：返回一个id值，代表该异步过程。若失败则返回0值。
#### 特别说明：通常我们不应直接使用此方法，而是使用ks_future\<T>::post_delayed发起异步延时任务。
<br>
<br>


```C++
void try_unschedule(uint64_t id);
```
#### 描述：请求撤销先前schedule的异步过程。（但并不能保证能成功撤销）
#### 参数：
  - id: 指定待撤销的异步过程id值。
#### 特别说明：如果异步过程已经执行完毕、或正在被执行，则不能成功撤销。
<br>
<br>


```C++
void atfork_prepare();
void atfork_parent();
void atfork_child();
```
#### 描述：fork辅助支持。
#### 特别说明：仅UNIX类系统下有效。
<br>
<br>
<br>
<br>


# 另请参阅
  - [HOME](HOME.md)
  - [ks_future\<T>](ks_future.md)
