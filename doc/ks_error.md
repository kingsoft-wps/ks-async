# `class ks_error`

# 说明

表示一个错误值。
<br>
<br>
<br>


# 静态成员方法

```C++
static ks_error of(HRESULT code);
```
<br>
<br>


# 一般成员方法

```C++
template <class T>
ks_error with_payload(T&& payload) const;
```

```C++
HRESULT get_code() const;

template <class T>
const T& get_payload() const;
```
<br>
<br>


# 预定义的错误值（静态成员方法）

```C++
static ks_error unexpected_error();
```
#### 描述：意外错误，错误码为0xFF3C0001。
<br>

```C++
static ks_error was_timeout_error();
```
#### 描述：异步过程超时，错误码为0xFF3C0002。
<br>

```C++
static ks_error was_cancelled_error();
```
#### 描述：异步过程被取消，错误码为0xFF3C0003。
<br>

```C++
static ks_error was_terminated_error();
```
#### 描述：异步环境（套间）已终止，错误码为0xFF3C0004。
<br>
<br>
<br>
<br>



# 另请参阅
  - [HOME](HOME.md)
  - [ks_result\<T>](ks_result.md)
  - [ks_future\<T>](ks_future.md)
  - [ks_promise\<T>](ks_promise.md)
