# `template <Class T>` <br> `class ks_future<T>`

# 说明

一个ks_future\<T>对象表示将来会产生（或当前即已就绪）的一个值或错误，其中值的类型为T，而错误的类型统一为ks_error。

比如，一个典型的情景是发起一个异步调用，这就可以用一个ks_future对象，当异步过程完成时，该ks_future的值或错误即产生（一般用值表示成功，错误表示失败）。

显然，ks_future对象是有状态的：

| 状态                    | 子状态                         | 说明                                           |
| ----------------------- | ----------------------------- | ---------------------------------------------- |
| not-completed（未完成） |                                | 还未完成                                       |
| completed（已完成）     | succ（成功） <br> error（错误） | 已完成，且成功了，有结果值 <br> 已完成，但失败了 |
|

ks_future的关键能力是支持任务编排，可以指定在ks_future完成时触发后续任务，从而使诸多异步任务有序调度。

ks_future是以链式调用的方式使用，形式如：`auto yFuture = xFuture.then(……).on_failure(……)`。

异步过程fn支持一个可选的入参ks_cancel_inspector*，通过这个接口可以检查cancel标志。对于长耗时的异步过程，应在适当位置主动检查cancel标志并处理（中止当前异步过程）。


<br>

*注1：ks_future的底层实现依赖ks_raw_future。通常，我们应使用ks_future\<T>，而不要去使用ks_raw_future，因为ks_future\<T>利用模板技术实现了类型强约束，更加方便可靠。*

*注2：与std::future不同，业务逻辑中通常并不需要直接持有和管理ks_future实例，异步过程一旦发起，就会按部就班的执行，ks_future实例的生命期会自动保持。*

*注3：[ks_future_util](#kfutureutil工具集) 提供了其他一些ks_future工具方法。*

<br>
<br>
<br>


# 静态成员方法

```C++
static ks_future<T> resolved(const T& value);
```
#### 描述：创建一个立即完成的ks_future对象，表示一个 “值”。
#### 参数：
  - value: 直接指定ks_future表示的值。
#### 返回值：新ks_future对象。
#### 特别说明：若T为void，则要求不传value参数、或传nothing。
<br>

```C++
static ks_future<T> rejected(ks_error error);
```
#### 描述：创建一个立即完成的ks_future对象，表示一个 “错误”。
#### 参数：
  - error: 直接指定ks_future表示的错误。
#### 返回值：新ks_future对象。
<br>
<br>


```C++
static ks_future<T> post(ks_apartment* apartment, function<T()> task_fn, const ks_async_context& context = {});
static ks_future<T> post(ks_apartment* apartment, function<ks_result<T>()> task_fn, const ks_async_context& context = {});
static ks_future<T> post(ks_apartment* apartment, function<ks_result<T>(ks_cancel_inspector*)> task_fn, const ks_async_context& context = {});
```
#### 描述：发起一个异步任务，此任务将在指定apartment套间中被执行。
#### 参数：
  - apartment: 指定异步任务执行时所在套间。若传nullptr，则使用default_mta套间。
  - task_fn: 异步任务函数，返回值类型为T或ks_result\<T>。（入参ks_cancel_inspector*可选）
  - context: 异步任务执行时所需上下文。
#### 返回值：新ks_future对象。
<br>

```C++
static ks_future<T> post_delayed(ks_apartment* apartment, function<T()> task_fn, int64_t delay, const ks_async_context& context = {});
static ks_future<T> post_delayed(ks_apartment* apartment, function<ks_result<T>()> task_fn, int64_t delay, const ks_async_context& context = {});
static ks_future<T> post_delayed(ks_apartment* apartment, function<ks_result<T>(ks_cancel_inspector*)> task_fn, int64_t delay, const ks_async_context& context = {});
```
#### 描述：发起一个延时的异步任务（相当于一个单次timer），此任务将在指定apartment套间中被执行。
#### 参数：
  - apartment: 指定异步任务执行时所在套间。若传nullptr，则使用default_mta套间。
  - task_fn: 异步任务函数，返回值类型为T或ks_result\<T>。（入参ks_cancel_inspector*可选）
  - delay: 延时时长，单位是毫秒（ms）。
  - context: 异步任务执行时所需上下文。
#### 返回值：新ks_future对象。
<br>

```C++
static ks_future<T> post_pending(ks_apartment* apartment, function<T()> task_fn, ks_pending_trigger* trigger, const ks_async_context& context = {});
static ks_future<T> post_pending(ks_apartment* apartment, function<ks_result<T>()> task_fn, ks_pending_trigger* trigger, const ks_async_context& context = {});
static ks_future<T> post_pending(ks_apartment* apartment, function<ks_result<T>(ks_cancel_inspector*)> task_fn, ks_pending_trigger* trigger, const ks_async_context& context = {});
```
#### 描述：发起一个挂起的异步任务（以避免异步任务被立即被调度执行），此任务将在指定apartment套间中被执行。
#### 参数：
  - apartment: 指定异步任务执行时所在套间。若传nullptr，则使用default_mta套间。
  - task_fn: 异步任务函数，返回值类型为T或ks_result\<T>。（入参ks_cancel_inspector*可选）
  - trigger: 异步任务触发器。异步任务将在trigger->start()后才会被触发执行。
  - context: 异步任务执行时所需上下文。
#### 返回值：新ks_future对象。
<br>
<br>



# 一般成员方法

```C++
template <class R>
ks_future<R> then<R>(ks_apartment* apartment, function<R(const T&)> fn, const ks_async_context& context = {});
template <class R>
ks_future<R> then<R>(ks_apartment* apartment, function<ks_result|ks_future<R>(const T&)> fn, const ks_async_context& context = {});
template <class R>
ks_future<R> then<R>(ks_apartment* apartment, function<ks_result<R>(const T&, ks_cancel_inspector*)> fn, const ks_async_context& context = {});
```
#### 描述：仅当this成功时，执行fn函数进行值变换，返回新的R类型结果。此函数将在指定apartment套间中被执行。
#### 模板参数：
  - R: 约定函数返回值类型为ks_future\<R>。
#### 参数：
  - apartment: 指定异步函数执行时所在套间。若传nullptr，则使用default_mta套间。
  - fn: 待执行的then异步函数，其入参类型为T，返回值类型为R或ks_result\<R>。（入参ks_cancel_inspector*可选）
  - context: 异步函数执行时所需上下文。
#### 返回值：新ks_future\<R>对象。
#### 特别说明：若R为void，则fn返回值类型要求为 `void` 或 `ks_result<void>`。
<br>

```C++
template <class R>
ks_future<R> transform<R>(ks_apartment* apartment, function<R(const ks_result<T>&)> fn, const ks_async_context& context = {});
template <class R>
ks_future<R> transform<R>(ks_apartment* apartment, function<ks_result<R>(const ks_result<T>&)> fn, const ks_async_context& context = {});
template <class R>
ks_future<R> transform<R>(ks_apartment* apartment, function<ks_result<R>(const ks_result<T>&, ks_cancel_inspector*)> fn, const ks_async_context& context = {});
```
#### 描述：当this完成时（无论成功/失败），执行fn函数进行值变换，返回新的R类型结果。此函数将在指定apartment套间中被执行。
#### 模板参数：
  - R: 约定函数返回值类型为ks_future\<R>。
#### 参数：
  - apartment: 指定异步函数执行时所在套间。若传nullptr，则使用default_mta套间。
  - fn: 待执行的transform异步函数，其入参类型为ks_result\<T>，返回值类型为R或ks_result\<R>。（入参ks_cancel_inspector*可选）
  - context: 异步函数执行时所需上下文。
#### 返回值：新ks_future\<R>对象。
#### 特别说明：若R为void，则fn返回值类型要求为 `void` 或 `ks_result<void>`。
<br>
<br>


```C++
template <class R>
ks_future<R> flat_then<R>(ks_apartment* apartment, function<ks_future<R>(const T&)> fn, const ks_async_context& context = {});
template <class R>
ks_future<R> flat_then<R>(ks_apartment* apartment, function<ks_future<R>(const T&, ks_cancel_inspector*)> fn, const ks_async_context& context = {});
```
#### 描述：仅当this成功时，执行fn函数进行值变换，返回新的R类型结果。此函数将在指定apartment套间中被执行。
#### 模板参数：
  - R: 约定函数返回值类型为ks_future\<R>。
#### 参数：
  - apartment: 指定异步函数执行时所在套间。若传nullptr，则使用default_mta套间。
  - fn: 待执行的then异步函数，其入参类型为T，返回值类型为ks_future\<R>。（入参ks_cancel_inspector*可选）
  - context: 异步函数执行时所需上下文。
#### 返回值：新ks_future\<R>对象。
<br>

```C++
template <class R>
ks_future<R> flat_transform<R>(ks_apartment* apartment, function<ks_future<R>(const ks_result<T>&)> fn, const ks_async_context& context = {});
template <class R>
ks_future<R> flat_transform<R>(ks_apartment* apartment, function<ks_future<R>(const ks_result<T>&, ks_cancel_inspector*)> fn, const ks_async_context& context = {};
```
#### 描述：当this完成时（无论成功/失败），执行fn函数进行值变换，返回新的R类型结果。此函数将在指定apartment套间中被执行。
#### 模板参数：
  - R: 约定函数返回值类型为ks_future\<R>。
#### 参数：
  - apartment: 指定异步函数执行时所在套间。若传nullptr，则使用default_mta套间。
  - fn: 待执行的transform异步函数，其入参类型为ks_result\<T>，返回值类型为ks_future\<R>。（入参ks_cancel_inspector*可选）
  - context: 异步函数执行时所需上下文。
#### 返回值：新ks_future\<R>对象。
<br>
<br>


```C++
ks_future<T> on_success(ks_apartment* apartment, function<void(const T&)> fn, const ks_async_context& context = {});
```
#### 描述：仅当this成功时，执行fn函数进行值处理，但新ks_future的仍保持原结果。此函数将在指定apartment套间中被执行。
#### 参数：
  - apartment: 指定异步函数执行时所在套间。若传nullptr，则使用default_mta套间。
  - fn: 待执行的on_success异步函数，其入参类型为T，无返回值。
  - context: 异步函数执行时所需上下文。
#### 返回值：新ks_future\<T>对象，其值将与this相同。
<br>

```C++
ks_future<T> on_failure(ks_apartment* apartment, function<void(const ks_error&)> fn, const ks_async_context& context = {});
```
#### 描述：仅当this失败时，执行fn函数进行错误处理，但新ks_future的仍保持原结果。此函数将在指定apartment套间中被执行。
#### 参数：
  - apartment: 指定异步函数执行时所在套间。若传nullptr，则使用default_mta套间。
  - fn: 待执行的on_failure异步函数，其入参类型为ks_error，无返回值。
  - context: 异步函数执行时所需上下文。
#### 返回值：新ks_future\<T>对象，其值将与this相同。
<br>

```C++
ks_future<T> on_completion(ks_apartment* apartment, function<void(const ks_result<T>&)> fn, const ks_async_context& context = {});
```
#### 描述：当this完成时（无论成功/失败），执行fn函数进行结果处理，但新ks_future的仍保持原结果。此函数将在指定apartment套间中被执行。
#### 参数：
  - apartment: 指定异步函数执行时所在套间。若传nullptr，则使用default_mta套间。
  - fn: 待执行的on_completion异步函数，其入参类型为ks_result\<T>，无返回值。
  - context: 异步函数执行时所需上下文。
#### 返回值：新ks_future\<T>对象，其值将与this相同。
<br>
<br>

```C++
template <class R>
ks_future<R> map<R>(function<R(const T&)> fn);
```
#### 描述：将this的T类型的结果值经转换函数fn变换为R类型，得到一个新的ks_future<R>对象。
#### 模板参数：
  - R: 约定函数返回值类型为ks_future\<R>。
#### 参数：
  - fn: 值转换函数（T类型 => R类型）。
#### 返回值：新ks_future\<R>对象。
#### 特别说明：`f.map<R>(fn)` 相当于 `f.then<R>(..., [](const T& val) { return fn(val); })`，但要求fn必须是简单函数。
<br>

```C++
template <class R>
ks_future<R> map_value<R>(const R& other_value);
```
#### 描述：将this的T类型的结果值变换为R类型新值，得到一个新的ks_future<R>对象。
#### 模板参数：
  - R: 约定函数返回值类型为ks_future\<R>。
#### 参数：
  - other_value: 新值（R类型）。
#### 返回值：新ks_future\<R>对象。
#### 特别说明：`f.map_value<R>()` 相当于 `f.then<R>(..., [](const T& val) { return other_value; })`
<br>

```C++
template <class R>
ks_future<R> cast<R>();
```
#### 描述：将this的T类型的结果值转换为R类型，得到一个新的ks_future<R>对象。
#### 模板参数：
  - R: 约定函数返回值类型为ks_future\<R>。（但要求R必须与T类型兼容或一致）
#### 返回值：新ks_future\<R>对象。
#### 特别说明：`f.cast_to<R>()` 相当于 `f.then<R>(..., [](const T& val) { return R(val); })`
<br>
<br>


# 状态和结果查询/控制

```C++
bool is_completed() const;
```
#### 描述：查询this是否已完成。
#### 返回值：是否已完成。
#### 特别说明：慎用，因为通常我们更应该使用异步结果处置系列方法来运用ks_future。
<br>

```C++
ks_result<T> peek_result() const;
```
#### 描述：查询this当前的结果（有可能是未完成状态的）。
#### 返回值：当前的结果。
#### 特别说明：慎用，因为通常我们更应该使用异步结果处置系列方法来运用ks_future。
<br>

```C++
void try_cancel();
```
#### 描述：请求中止此异步过程。（但并不能保证能成功中止）
#### 特别说明：此方法会自动向上游回溯递归执行try_cancel，直至根future。<br>
在异步过程实现体中，可以调用`cancel_inspector->check_cancel()`方法检查对应的"取消"控制标志。<br>
慎用，因为决大多数异步过程实现者都会忽视此标志、或者根本就无法中止其执行过程。<br>
<br>

```C++
void set_timeout(int64_t timeout);
```
#### 描述：设置此异步过程的超时时间。（但并不能保证能成功中止）
#### 参数：
  - timeout: 超时时长，单位是毫秒（ms）。
#### 特别说明：当超时后，会向上游回溯递归尝试中止（类似于try_cancel），直至根future。
<br>
<br>
<br>



# 另请参阅
  - [HOME](HOME.md)
  - [ks_future_util](ks_future_util.md)
  - [ks_promise\<T>](ks_promise.md)
  - [ks_result\<T>](ks_result.md)
