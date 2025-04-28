# `template <Class T>` <br> `class ks_result<T>`

# 说明

一个ks_result对象用来表示一个异步结果，它或者是未完成的、或者表示一个T类型值（代表成功）、或者表示一个ks_error（代表失败）。ks_result\<T>相当于是一个 T类型值、ks_error、以及无数据（未完成状态） 三者的联合体（union）。

ks_future\<T>的异步完成结果即以一个ks_result\<T>对象表示。

ks_result还具有不可变性，其内包含的 “值” 是不可变的。（然而，非const的ks_result类型变量本身仍可以被赋值为一个新ks_result值，赋值后其内将包含一个全新的“值实例”，而非覆写旧“值实例”）

关于ks_error和ks_future，另请参阅[ks_error](ks_error.md)和[ks_future\<T>](ks_future.md)。

<br>

*注1：ks_result的底层实现依赖ks_raw_result和ks_raw_value。通常，我们应使用ks_result\<T>，而不要去使用ks_raw_result和ks_raw_value，因为ks_result\<T>利用模板技术实现了类型强约束，更加方便可靠。然而ks_error仍被ks_result直接使用以表示错误。*

<br>
<br>


# 构造方法

```C++
ks_result<T>::ks_result(const T& value);
```
#### 描述：构造一个有值ks_result对象，状态为已完成、且有值。
#### 参数：
  - value: 指定的值。
#### 特别说明：若T为void，则要求不传value参数、或传nothing。
<br>

```C++
ks_result<T>::ks_result(ks_error error);
```
#### 描述：构造一个错误ks_result对象，状态为已完成、且错误。
#### 参数：
  - error: 指定的错误。
<br>
<br>


# 一般成员方法

```C++
bool is_completed() const;
```
#### 描述：判定this状态是否为已完成。
#### 返回值：状态是否为已完成。
<br>

```C++
bool is_value() const;
```
#### 描述：判定this状态是否为已完成状态、且有值。
#### 返回值：是否为已完成状态、且有值。
<br>

```C++
bool is_error() const;
```
#### 描述：判定this状态是否为已完成状态、且错误。
#### 返回值：是否为已完成状态、且错误。
<br>

```C++
const T& to_value() const noexcept(false);
```
#### 描述：获取this内包含的T类型的值，要求this必须为已完成状态、且有值。
#### 返回值：this内包含的T类型的值。
#### 异常：若状态不正确，则抛出异常（当前包含的error值或unexcepted_error）。
#### 特别说明：若T为void，为方便使用，此方法返回值为nothing。
<br>

```C++
ks_error to_error() const noexcept(false);
```
#### 描述：获取this内包含的错误，要求this必须为已完成状态、且错误。
#### 返回值：this内包含的错误。
#### 异常：若状态不正确，则抛出异常（unexcepted_error）。
<br>

```C++
template <class R>
ks_result<R> cast<R>();
```
#### 描述：将this的T类型的结果值转换为R类型，得到一个新的ks_result<R>对象。
#### 模板参数：
  - R: 约定函数返回值类型为ks_result\<R>。（但要求R必须与T类型兼容或一致）
#### 返回值：新ks_result\<R>对象。
<br>

```C++
template <class R>
ks_result<R> map<R>(function<R(const T&)> fn);
```
#### 描述：将this的T类型的结果值经转换函数fn变换为R类型，得到一个新的ks_result<R>对象。
#### 模板参数：
  - R: 约定函数返回值类型为ks_result\<R>。
#### 参数：
  - fn: 值转换函数（T类型 => R类型）。
#### 返回值：新ks_result\<R>对象。
<br>

```C++
template <class R>
ks_result<R> map_value<R>(const R& other_value);
```
#### 描述：将this的T类型的结果值变换为R类型新值，得到一个新的ks_result<R>对象。
#### 模板参数：
  - R: 约定函数返回值类型为ks_result\<R>。
#### 参数：
  - other_value: 新值（R类型）。
#### 返回值：新ks_result\<R>对象。
<br>
<br>
<br>
<br>


# 另请参阅
  - [HOME](HOME.md)
  - [ks_error](ks_error.md)
  - [ks_future\<T>](ks_future.md)
