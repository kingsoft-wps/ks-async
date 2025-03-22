# `class ks_async_flow & ks_async_flow_ptr`

# 说明

高级异步任务编排器。

<br>
<br>


# 静态成员方法

```C++
static ks_async_flow_ptr ks_async_flow::create();
```
#### 描述：创建一个实例。
<br>
<br>


# 一般成员方法

```C++
bool add_task<T>(
    const char* name_and_dependencies,
    ks_apartment* apartment, std::function<T(const ks_async_flow_ptr& flow)>&& fn, const ks_async_context& context = {});
bool add_task<T>(
    const char* name_and_dependencies,
    ks_apartment* apartment, std::function<ks_result<T>(const ks_async_flow_ptr& flow)>&& fn, const ks_async_context& context = {});
bool add_task<T>(
    const char* name_and_dependencies,
    ks_apartment* apartment, std::function<ks_future<T>(const ks_async_flow_ptr& flow)>&& fn, const ks_async_context& context = {});
```
#### 描述：添加任务。
#### 特别说明：name_and_dependencies指定任务名称和前置依赖清单，其格式为："t5: t1, t2, t3, t4"。
<br>
<br>


```C++
uint64_t add_flow_running_observer(ks_apartment* apartment, std::function<void(const ks_async_flow_ptr& flow)>&& fn, const ks_async_context& context);
uint64_t add_flow_completed_observer(ks_apartment* apartment, std::function<void(const ks_async_flow_ptr& flow, const ks_error& error)>&& fn, const ks_async_context& context);

uint64_t add_task_running_observer(const char* task_name_pattern, ks_apartment* apartment, std::function<void(const ks_async_flow_ptr& flow, const char* task_name)>&& fn, const ks_async_context& context);
uint64_t add_task_completed_observer(const char* task_name_pattern, ks_apartment* apartment, std::function<void(const ks_async_flow_ptr& flow, const char* task_name, const ks_error& error)>&& fn, const ks_async_context& context);

void remove_observer(uint64_t id);
```
#### 描述：添加/移除观察者。
<br>
<br>


```C++
bool start();
void try_cancel();
void wait(); //deprecated
```
#### 描述：开始、取消、等待。
#### 特别说明：wait目前为deprecated方法，禁止使用。
<br>
<br>


```C++
bool is_flow_completed();
bool is_task_completed(const char* task_name);

ks_result<T> get_task_result<T>(const char* task_name);
T get_task_result_value<T>(const char* task_name);
```
#### 描述：查询状态和结果。
<br>

```C++
ks_error get_last_error();
ks_error get_task_error(const char* task_name);

std::string get_failed_task_name();
```
#### 描述：查询错误信息。
#### 特别说明：应在相应任务完成后调用。
<br>
<br>


```C++
ks_future<ks_async_flow_ptr> get_flow_future();
```
#### 描述：获得代表flow的一个future。
<br>
<br>


```C++
void set_user_data(const char* name, const T& value);
T get_user_data(const char* name);
```
#### 描述：设置/获取user-data。
<br>
<br>


```C++
void set_default_apartment(ks_apartment* apartment);
```
#### 描述：设置默认套间，初始为default-mta。
<br>

```C++
void set_j(size_t j);
```
#### 描述：设置并发度，初始无限制。
<br>
<br>

