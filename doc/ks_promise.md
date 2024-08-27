# `template <Class T>` <br> `class ks_promise<T>`

# 说明

一个ks_promise\<T>对象表示一个未完成（未决）的结果。可以用一个值来reslove，也可以用一个错误来reject。

调用 `ks_promise<T>::create()` 方法可以创建一个新ks_promise对象，新的ks_promise\<T>对象的初始状态是未完成（未决）的。

ks_promise提供了一个与其关联的ks_future对象，通过此ks_future对象即可对ks_promise的结果进行各种处置。

通常，每个被创建的ks_promise对象都应该最终被完成（reslove或reject），以避免相关ks_future永不能完成，从而产生意料外的资源泄漏。

若ks_promise状态为已完成（即已调用过resolve或reject），则后续再次调用resolve或reject的话不会发生任何作用。

可以看出，ks_promise可用于一个外部的异步调用，例如：
```C++
ks_future<ks_stdptr<IDataBuffer>> async_download(const std::string& url) {
    auto promise = ks_promise<ks_stdptr<IDataBuffer>>::create();
    netlib::do_async_download(url, [promise](IDataBuffer* pDataBuffer) {
        if (hr == 0) 
            promise.resolve(ks_stdptr<IDataBuffer>(pDataBuffer));
        else
            promise.reject(ks_error::of(...));
    });
    
    return promise.get_future();
}
```

<br>

*注1：ks_promise的底层实现依赖ks_raw_promise。通常，我们应使用ks_promise\<T>，而不要去使用ks_raw_promise，因为ks_promise\<T>利用模板技术实现了类型强约束，更加方便可靠。*

<br>
<br>


# 静态成员方法

```C++
static ks_promise<T> create();
```
#### 描述：创建一个ks_promise对象。
#### 特别说明：新ks_promise对象应该最终完成（reslove或reject），以避免相关ks_future永不能完成，从而产生意料外的资源泄漏。
<br>
<br>
<br>


# 一般成员方法

```C++
ks_future<T> get_future() const;
```
#### 描述：获取与this关联的ks_future对象。
#### 返回值：关联的ks_future对象。
<br>

```C++
void resolve(const T& value) const;
```
#### 描述：成功完成。
#### 参数：
  - value: 结果值。
#### 返回值：无。
#### 特别说明：ks_promise对象应该最终完成（reslove或reject），以避免相关ks_future永不能完成，从而产生意料外的资源泄漏。
#### 特别说明：若T为void，则要求不传value参数、或传nothing。
<br>

```C++
void reject(ks_error error) const;
```
#### 描述：失败完成。
#### 参数：
  - error: 错误。
#### 返回值：无。
#### 特别说明：ks_promise对象应该最终完成（reslove或reject），以避免相关ks_future永不能完成，从而产生意料外的资源泄漏。
<br>
<br>
<br>




# 另请参阅
  - [HOME](HOME.md)
  - [ks_future\<T>](ks_future.md)
  - [ks_result\<T>](ks_result.md)
