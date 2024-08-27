# `class ks_async_controller`

# 说明

异步过程控制器，配合context使用。在context构造时要求传入一个controller（可选）。

使用controller，可以请求中止相关异步过程。

<br>
<br>


# 构造方法

```C++
ks_async_controller();
```
#### 描述：无。
<br>
<br>


# 一般成员方法

```C++
void try_cancel_all();
```
#### 描述：请求中止所有相关异步过程。（但并不能保证能成功中止）
#### 返回值：无。
<br>

```C++
bool has_pending_futures() const;
```
#### 描述：检查当前是否存在还未完成的相关异步过程。
#### 返回值：是否存在还未完成的相关异步过程。
<br>

```C++
void wait_pending_futures_done() const;
```
#### 描述：等待当前未完成的相关异步过程全部完成。
#### 特别说明：**慎用，使用不当可能会造成死锁或卡顿！** <br>
与ks_raw_future::wait情况类似，甚至更危险，因为这里连基本的跨apartment检查都没有，极易死锁。<br>
<br>
<br>



# 另请参阅
  - [HOME](HOME.md)
  - [ks_async_context](ks_async_context.md)
  - [ks_future\<T>](ks_future.md)
  