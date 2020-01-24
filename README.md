# How to manage the Execution Sequence in an asynchronous task - 如何管理一个异步任务的执行顺序.

## 1. what is the reactive programming - 什么是响应式编程.

引用自维基百科
> [In computing, reactive programming is a declarative programming paradigm concerned with data streams and the propagation of change. ][1]
> 
> 
> 在计算机领域，响应式编程一种编程范式，是对于数据流动，以及对这种流动所产生变化的描述。

在异步编程环境下，如何有效地同步以及管理程序的前后逻辑关系，这对于任何人都是一种挑战。因为我们无法预测任何一个事件具体发生时间，比如我们无法预测一个TCP包什么时候可以到达服务端，我们只能注册一些回调函数或者观察者对象，来监视这一个事件的发生，并将发生的事件在系统内部广播出去。

传统的方式是，我们会编写一些轮询方法主动查询一个事件发生的标志，来发现并读取一个事件。

```
// C++: code fragment.

mutable bool isRaised = false;

// register a callback function.

event.registerObserver(new Observer{
    [](){isRaised = true;} 
});

event.watching();

while(!isRaised){
    sleepMs(30);
}

// the other procedures.
```

像上述代码段描述，我们会注册一个观察者。然后主动调用一个sleepMs(30)方法，以每30ms为间隔的方式来查询该事件是否发生。更进一步的版本中，我们会设置一个计数器，来计算总的超时时间，以便设置一个总的超时时间，如果我们等不到我们所要监听的事件时我们依然能够退出循环，而不至于因为事件不发生而产生系统上的死锁问题。

```
// a improvment version.

mutable bool isRaised = false;
int nCounter = 0;

// register a callback function.

event.registerObserver(new Observer([](){isRaised = true;}) );

event.watching();

while(!isRaised) {
    if(nCounter++ == 30){
      // notify timeout.   
       return;
    }

    sleepMs(30);
}

// the other procedures.
```

那么上面的代码还有哪些问题呢，它是否就完美了呢？

答案显然不是。因为我们总是需要主动调用sleepMs(30)将当前的进程或者线程挂起30ms，然后再查询isRaised标志来判断上述事件是否发生了。如果这个事件在我们主动延迟的30ms中发生了呢？那么我们是否就无法捕获。如果幸运地，在我们监听的这段时间里事件刚好发生了，但发生了两次，那么我们的代码是否就遗漏了一次事件？

那么有什么更好的方案吗？

## 2. The Deferred Class.

借鉴于响应式编程范式，我们借鉴了 Javascript语言中的 **Promise** 的接口设计，基于Qt Framework实现了一套类似的异步顺序执行流程框架实现，并命名为
[The DeferredTask][2].

* 下面是调用示例:

```
    QScopedPointer<QEventLoop> _looper(new QEventLoop);
    QSharedPointer<DeferredTask<QByteArray,QString>> deferredTask(new DeferredTask<QByteArray, QString>());

    TcpConnectionDelegate tcpConnection;

    QObject::connect(&tcpConnection, &TcpConnectionDelegate::sigHasReadyClient,[&](QTcpSocket* _tcpClient){
            qDebug()<<"on new connection "<<_tcpClient;
            deferredTask->observeOn([_tcpClient](DeferredTask<QByteArray, QString>::Deferred &def){
                 QByteArray _data = _tcpClient->readAll();
                 def.slove(_data);
            })
            .then([](DeferredTask<QByteArray, QString>::Deferred &def, QByteArray _incomingdata){
                qDebug()<<"CALL  def= "<<&def;
                qDebug()<<"CALL  _incomingdata "<<_incomingdata;
                _trace_tcp_package(std::move(def), std::move(_incomingdata));

            })
            .then([_tcpClient](DeferredTask<QByteArray, QString>::Deferred &def, QByteArray _incomingdata){
                _tcpClient->write(_incomingdata);
                _tcpClient->flush();
                _tcpClient->close();
            })
            .catching([](QString errorMessage){
                LOGGING_LEVEL("ERROR")<<"errorMessage "<<errorMessage;
            })
           .start();
    });

    QObject::connect(&tcpConnection, &TcpConnectionDelegate::sigOnError,[&](QAbstractSocket::SocketError errors){
            qWarning()<<"on accept error code "<<errors;
            _looper->exit(-1);
    });

    if(!tcpConnection.listen(QHostAddress::Any, 5544)){
        qFatal()<<"listen 5544 failure! server exit from listen loop!";
        return;
    }

    _looper->exec();
```
* 代码流程解析；
  1. 创建一个 **DeferredTask**，然后通过**observeOn()**方法绑定到**TcpConnectionDelegate::sigHasReadyClient**的事件观察者中。
  2. **DeferredTask.observeOn()**方法将产生一个**Deferred**对象，将注册到**observeOn()**，以及**then()**,**catching()**的回调方法保存到其内部，
    当一个新的**QTcpSocket** 被QTcpServer获得时，在其注册给observeOn

   ```
   [_tcpClient](DeferredTask<QByteArray, QString>::Deferred &def) {
                 QByteArray _data = _tcpClient->readAll();
                 def.slove(_data);
      }
   ```

   的lambda函数将被执行，读取tcp缓存内的所有数据，并通过**def.slove()**传递给执行链中的第一个**then()**所注册的lambda函数。
  3. 当该调用成功执行完成后，**DeferredTask** 会继续调用 **then()** 方法绑定的回调对象，顺序执行Task链中所有需要监听的任务，直到最后一个**then()**所绑定的lambda函数。
  4. 当 **Deferred**对象上所有**then()**所绑定的lambda函数都被执行完成，该对象将被自动释放。
  5. **DeferredTask.observeOn()** 的异步执行特性使得，不论 **QTcpSocket** 何时连接上服务，任何一个新**QTcpClient**对象都将被绑定到一个新创建的**Deferred**对象上执行，而不会阻塞 **QTcpServer**的 Accept()方法。从而提升系统的吞吐性能。



## 3. TODO

1. 模仿 jQuery.deferred 添加 done()方法来结束流程；
2. 借鉴 RxJava 添加 Schedule()将Deferred对象分配到不同的线程上执行，提高并发能力；


[1]:https://en.wikipedia.org/wiki/Reactive_programming
[2]:https://github.com/turbin/DeferredTask