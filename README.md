# Promise: An Asynchronous Promgramming Paradigm - Promise:一种异步编程范式 

## 1. About Promise - 关于Promise的介绍.

Promise是Javascript的一种异步编程范式。在Javascript 中使用Promise可以很简单的实现异步事件的顺序调用的逻辑。

它的调用方式像这样:
```
var promise1 = new Promise(function(resolve, reject) {
  setTimeout(function() {
    resolve('foo');
  }, 300);
});

promise1.then(function(value) {
  console.log(value);
});

console.log(promise1);
// expected output: [object Promise]
```

* a. 这里我们创建了一个Promise对象，将其绑定监听一个定时器事件,定时器的超时时间设置为300 ms.
```
var promise1 = new Promise(function(resolve, reject) {
  setTimeout(function() {
    resolve('foo');
  }, 300);
});
```

* b. 当定时器事件触发时, **resolve('foo')** 这个方法将被调用.

* c. **resolve('foo')** 被调用后，程序将callback我们注册在 **Promise.then()** 的方法.此时我们可以从这个方法中得到resolve传递过来的参数 **'foo'** .

```
promise1.then(function(value) {
  console.log(value);
  // 这里将打印输出 'foo'
});
```
* d. promise 还可以完成下面的链式调用，通过异步调用方式来实现代码顺序执行的逻辑。
```
Promise.resolve("foo")
  // 1. 接收 "foo" 并与 "bar" 拼接，并将其结果做为下一个 resolve 返回。
  .then(function(string) {
    return new Promise(function(resolve, reject) {
      setTimeout(function() {
        string += 'bar';
        resolve(string);
      }, 1);
    });
  })
  // 2. 接收 "foobar", 放入一个异步函数中处理该字符串
  // 并将其打印到控制台中, 但是不将处理后的字符串返回到下一个。
  .then(function(string) {
    setTimeout(function() {
      string += 'baz';
      console.log(string);
    }, 1)
    return string;
  })
  // 3. 打印本节中代码将如何运行的帮助消息，
  // 字符串实际上是由上一个回调函数之前的那块异步代码处理的。
  .then(function(string) {
    console.log("Last Then:  oops... didn't bother to instantiate and return " +
                "a promise in the prior then so the sequence may be a bit " +
                "surprising");

    // 注意 `string` 这时不会存在 'baz'。
    // 因为这是发生在我们通过setTimeout模拟的异步函数中。
    console.log(string);
  });

// logs, in order:
// Last Then: oops... didn't bother to instantiate and return a promise in the prior then so the sequence may be a bit surprising
// foobar
// foobarbaz
```

## 2. Introduction the implementation based on Qt & C++ - 基于Qt/C++的实现简介。

在Qt应用开发过程中，我们引入了异步执行架构模型。但是实际开发场景中，交易业务处理流程是一个顺序执行流程。
其特点是后一步骤的执行输入依赖前一步骤的执行输出。所以我们很需要类似 **Promise**的实现。
在开发过程中，我们也基于Qt的Signal-Slot机制实现了一套简单的 **Promise** 调用。

* 下面是调用示例:

```
/// step 1. create a data service then start it.
DataVisitService* dataVisitingService 
                    = DataVisitService::Builder(DataVisitService::Builder::DATA_SOURCE::SOURCE_FROME_DATABASE).building();

dataVisitingService->service_initialize();
dataVisitingService->service_start();

// create a deferred task，then observed the data service event then pass to onSlove callback.

QSharedPointer<DeferredTask<QString>>  deferred(new DeferredTask<QString>);

deferred->ObserveOn([=](DeferredTask<QString>::Deferred& def) {
      DeferredTask<QString>::Deferred&& _dup_deferred
              = std::forward<DeferredTask<QString>::Deferred>(def);

      DeferredTask<QString>::Deferred* _p = &def;
      LOGGING_LEVEL("debug")<<"DeferredTask<QString>::Deferred "<<_p;

      dataVisitingService->getData("MerchaintName",
          new IDataVisitor::DataVisitResponseObserver(
              [&_dup_deferred](const QVariantMap& response){ ///onCompleted
                  __VERIFY(response.contains("MerchaintName"));
                  LOGGING_LEVEL("debug")<<"response "<<response;
                  _dup_deferred.slove(response["MerchaintName"]);
              },

              [&_dup_deferred](int error){
                  LOGGING_LEVEL("debug")<<"error "<<error;
                  _dup_deferred.reject("error !");
              }));
      }
).onSlove([=](QString value) {
    LOGGING_LEVEL("debug")<<"value "<<value;
    LOGGING_LEVEL("debug")<<"@here";
}).then([=](DeferredTask<QString>::Deferred& def) {
    DeferredTask<QString>::Deferred&& _dup_deferred
                          = std::forward<DeferredTask<QString>::Deferred>(def);

    DeferredTask<QString>::Deferred* _p = &def;
    LOGGING_LEVEL("debug")<<"DeferredTask<QString>::Deferred "<<_p;

    dataVisitingService->getData("SerialNo",
        new IDataVisitor::DataVisitResponseObserver(
            [&_dup_deferred](const QVariantMap& response){ ///onCompleted
                __VERIFY(response.contains("SerialNo"));
                LOGGING_LEVEL("debug")<<"response "<<response;
                _dup_deferred.slove(response["SerialNo"]);
            },

            [&_dup_deferred](int error){
                LOGGING_LEVEL("debug")<<"error "<<error;
                _dup_deferred.reject("error !");
            }));
        }
).onSlove([=](QString value){
    LOGGING_LEVEL("debug")<<"value "<<value; // expect SerialNo
    LOGGING_LEVEL("debug")<<"@here";
    looper->quit();
    dataVisitingService->service_shutdown();
}).start(); // start this task. 
```
* 代码流程解析
  1. 创建一个**DataVisitorService** 并启动这个服务；
  2. 创建一个 **DeferredTask**，然后将其**ObserveOn()**方法绑定到**DataVisitorService.getData()**的事件观察者中。
  3. 当 **getData** 通过其事件观察者 **DataVisitResponseObserver** 获得到DataVisitorService的返回信息后,
     调用 **Deferred.slove()** 将获得到返回信息传递到 **DeferredTask.onSlove()** 中。
  4. 当该调用成功执行完成后，**DeferredTask** 会继续调用 **then()** 方法绑定的回调对象，顺序执行Task链中所有需要监听的任务。

上述的 **DeferredTask** 就完成了异步调用场景中顺序执行逻辑。

DeferredTask 实现,请参考[Deferred Task](https://github.com/turbin/DeferredTask/blob/master/deferredtask.hpp)
