# `class ks_async_context`

# 说明

异步过程上下文。context可以指定异步任务选项，还可以维持一组智能指针。

所有涉及异步过程的方法都会明确要求调用中传入一个context参数，该context内所持对象在相应异步过程执行完毕前保持有效。

<br>
<br>
<br>


# 新实例

```C++
ks_async_context make_async_context();
```
#### 描述：构造一个新实例。
#### 描述：构造时，指定一个domain_ptr智能指针（以保持其生命期），以及一个control控制器指针。
#### 参数：
  - smart_ptr: 任意对象智能指针，一般为this对象的智能指针，其常见类型为ks_stdptr或std::shared_ptr或std::weak_ptr。可以传nullptr。
  - controller: 关联异步过程控制器。可以传nullptr。
#### 特别说明：实际上这是一个宏，以便额外记录一些调试用信息。
<br>
<br>


# 一般成员方法

```C++
void bind_owner<SMART_PTR>(SMART_PTR owner_ptr);
```
#### 描述：绑定一个owner智能指针，其常见类型为std::shared_ptr或std::weak_ptr或ks_stdptr。
#### 参数：
  - smart_ptr: owner对象智能指针。
#### 特别说明：因owner为智能指针，且context会被保证在异步过程执行完成前不失效，故owner也不失效。<br>
特别地，若owner为weak_ptr，则异步过程被执行前会自动lock，且保持至执行完成；若lock返回为nullptr，则自动cancel。
<br>

```C++
void bind_controller(const ks_async_controller* controller);
```
#### 描述：绑定一个controller。
#### 参数：
  - controller: controller对象。
<br>

```C++
void set_parent(const ks_async_context& parent, bool inherit_common_props = true);
```
#### 描述：设置父context。
#### 参数：
  - parent：父context。
  - inherit_common_props: 是否继承parent的一般属性。（目前即为priority属性）
<br>

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
  