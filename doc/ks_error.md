# `class ks_error`

# 说明

表示一个错误值。
<br>
<br>
<br>


# 构造方法

```C++
ks_error::ks_error();
```
<br>

```C++
static ks_error of(HRESULT code);
```
<br>
<br>


# 一般成员方法

```C++
ks_error with_payload<T>(const T& payload) const;
```

```C++
HRESULT get_code() const;

const T& get_payload<T>() const;
```
<br>
<br>


# 预定义的错误值（静态成员方法）

```C++
static ks_error unexpected_error();
```
#### 描述：意外错误，错误码为0xFF338001。
<br>

```C++
static ks_error timeout_error();
```
#### 描述：异步过程超时，错误码为0xFF338002。
<br>

```C++
static ks_error cancelled_error();
```
#### 描述：异步过程被取消，错误码为0xFF338003。
<br>

```C++
static ks_error terminated_error();
```
#### 描述：异步环境（套间）已终止，错误码为0xFF338004。
<br>
<br>

```C++
static ks_error general_error();
```
#### 描述：一般错误，错误码为0xFF339001。
<br>

```C++
static ks_error eof_error();
```
#### 描述：EOF错误，错误码为0xFF339002。
<br>

```C++
static ks_error arg_error();
```
#### 描述：参数错误，错误码为0xFF339003。
<br>

```C++
static ks_error data_error();
```
#### 描述：数据错误，错误码为0xFF339004。
<br>

```C++
static ks_error status_error();
```
#### 描述：状态错误，错误码为0xFF339005。
<br>
<br>



# 另请参阅
  - [HOME](HOME.md)
  - [ks_result\<T>](ks_result.md)
  - [ks_future\<T>](ks_future.md)
  - [ks_promise\<T>](ks_promise.md)
