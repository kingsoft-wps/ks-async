# `class ks_async_context`

# 说明

异步过程上下文。context可以指定异步任务选项，还可以维持一组智能指针。

所有涉及异步过程的方法都会明确要求调用中传入一个context参数，该context内所持对象在相应异步过程执行完毕前保持有效。

<br>
<br>
<br>


# 构造方法

```C++
template <class SMART_PTR>
ks_async_context(SMART_PTR smart_ptr, ks_async_controller* controller);
```
#### 描述：构造时，指定一个domain_ptr智能指针（以保持其生命期），以及一个control控制器指针。
#### 参数：
  - smart_ptr: 任意对象智能指针，一般为this对象的智能指针，其常见类型为ks_stdptr或std::shared_ptr或std::weak_ptr。可以传nullptr。
  - controller: 关联异步过程控制器。可以传nullptr。
<br>
<br>


# 一般成员方法

```C++
void set_priority(int priority);
```
#### 描述：设置异步过程的优先级。
#### 参数：
  - priority: 异步过程的优先级。（默认是0）
<br>
<br>
<br>


# 另请参阅
  - [HOME](HOME.md)
  - [ks_async_controller](ks_async_controller.md)
  - [ks_future\<T>](ks_future.md)
  