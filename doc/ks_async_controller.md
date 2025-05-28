# `class ks_async_controller`

# 说明

异步过程控制器，配合context使用。在context构造时要求传入一个controller（可选）。

使用controller，可以请求中止相关异步过程。

<br>
<br>


# 构造方法

```C++
ks_async_controller::ks_async_controller();
```
#### 描述：无。
<br>
<br>


# 一般成员方法

```C++
void try_cancel();
```
#### 描述：请求取消（中止）所有相关异步过程。（但并不能保证能成功中止）
#### 返回值：无。
<br>

```C++
bool check_cancelled();
```
#### 描述：查询当前取消（中止）标志状态。
#### 返回值：当前取消（中止）标志状态。
<br>
<br>



# 另请参阅
  - [HOME](HOME.md)
  - [ks_async_context](ks_async_context.md)
  - [ks_future\<T>](ks_future.md)
  