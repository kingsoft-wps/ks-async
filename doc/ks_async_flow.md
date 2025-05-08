# `class ks_async_flow`

# 说明

高级异步任务编排器。

<br>
<br>


# 一般成员方法

```C++
void set_j(size_t j);
```
#### 描述：选项。
<br>
<br>


```C++
bool add_task<T>(
    const char* name_and_dependencies,
    ks_apartment* apartment, std::function<T(const ks_async_flow& flow)> fn, const ks_async_context& context = {});
bool add_task<T>(
    const char* name_and_dependencies,
    ks_apartment* apartment, std::function<ks_result<T>(const ks_async_flow& flow)> fn, const ks_async_context& context = {});
bool add_task<T>(
    const char* name_and_dependencies,
    ks_apartment* apartment, std::function<ks_future<T>(const ks_async_flow& flow)> fn, const ks_async_context& context = {});
```
#### 描述：添加任务。
#### 特别说明：name_and_dependencies指定任务名称和前置依赖清单，其格式为："t5: t1, t2, t3, t4"。
<br>
<br>


```C++
uint64_t add_flow_running_observer(ks_apartment* apartment, std::function<void(const ks_async_flow& flow)> fn, const ks_async_context& context);
uint64_t add_flow_completed_observer(ks_apartment* apartment, std::function<void(const ks_async_flow& flow, const ks_error& error)> fn, const ks_async_context& context);

uint64_t add_task_running_observer(const char* task_name_pattern, ks_apartment* apartment, std::function<void(const ks_async_flow& flow, const char* task_name)> fn, const ks_async_context& context);
uint64_t add_task_completed_observer(const char* task_name_pattern, ks_apartment* apartment, std::function<void(const ks_async_flow& flow, const char* task_name, const ks_error& error)> fn, const ks_async_context& context);

void remove_observer(uint64_t id);
```
#### 描述：添加/移除观察者。
<br>
<br>


```C++
T get_value<T>(const char* key);
```
#### 描述：根据key获取value。
#### 特别说明：get_value方法也用于获取task的结果值。
<br>

```C++
void put_custom_value<T>(const char* key, const T& value);
```
#### 描述：根据自定义的key设置value。
#### 特别说明：仅用于设置自定义键值对。相反的，各task的结果值会自动被记录，无需手工设置。
<br>
<br>


```C++
bool start();
void try_cancel();
```
#### 描述：开始、取消。
<br>
<br>


```C++
bool is_flow_running();
bool is_task_running(const char* task_name);
bool is_flow_completed();
bool is_task_completed(const char* task_name);
```
#### 描述：查询状态。
<br>

```C++
ks_error get_last_error();
std::string get_last_failed_task_name();
```
#### 描述：获取错误信息。
<br>

```C++
ks_error peek_task_error(const char* task_name);
ks_result<T> peek_task_result<T>(const char* task_name);
```
#### 描述：查询task当前的结果（有可能是未完成状态的）。
<br>

```C++
ks_future<T> get_task_future<T>(const char* task_name);
ks_future<ks_async_flow> get_flow_future();
```
#### 描述：获得代表task或flow的一个future。
<br>
<br>
