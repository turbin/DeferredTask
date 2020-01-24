#ifndef DEFERREDTASK_H
#define DEFERREDTASK_H
/*
    author: Turbine Yan
    verison: 1.01
*/
#include <QObject>
#include <QThread>
#include <QDebug>
#include <QRunnable>
#include <QQueue>
#include <QVariant>
#include <QMutex>
#include <QFuture>
#include <functional>
#include <QMap>
#include <memory>

//#include "logger.h"



namespace DeferredTaskPrivate {


class IDeferred
{
public:
    virtual void slove (QVariant  value)=0;
    virtual void reject(QVariant value)=0;
};

class SignalsHandler: public QObject
{
    Q_OBJECT
public :
    SignalsHandler(QObject* parent=0):QObject(parent){}

    virtual ~SignalsHandler(){}

signals:
    void sigCallObservee();
    void sigSloved(QVariant );
    void sigRejected(QVariant );
    void sigDestoryed();

public slots:

    void callObservee(){emit sigCallObservee();}
    void slove (QVariant  value){ emit sigSloved(value); }
    void reject(QVariant value){ emit sigRejected(value); }
    void release(){ emit sigDestoryed(); deleteLater();}
};


class SlotsHandler : public QObject
{
    Q_OBJECT
public:
    typedef std::function<void(void)>            ObserveeSlot;
    typedef std::function<void(QVariant&)>       OnSlovedSlot;
    typedef std::function<void(QVariant&)>       OnRejectedSlot;


    SlotsHandler(QObject* parent=0):QObject(parent),mSlotCallBack(new SlotCallback()){}

    void clearCallBack()
    {
        if(mSlotCallBack != nullptr) delete mSlotCallBack;
         mSlotCallBack = nullptr;
    }

    virtual ~SlotsHandler()
    {
        if(mSlotCallBack != nullptr) delete mSlotCallBack;

         mSlotCallBack = nullptr;
    }


    SlotsHandler& bindObservee(ObserveeSlot onObserveeCallback){
        mSlotCallBack->onObserveeCalled = std::move(onObserveeCallback);
        return *this;
    }

    SlotsHandler& bindOnSloved(OnSlovedSlot onSlovedCallback){
        mSlotCallBack->onSlovedCalled = std::move(onSlovedCallback);
        return *this;
    }


    SlotsHandler& bindOnRejected(OnRejectedSlot onRejectedCallback){
        mSlotCallBack->onRejectedCalled = std::move(onRejectedCallback);
        return *this;
    }

public slots:
    virtual void callObservee(){mSlotCallBack->onObserveeCalled();}
    virtual void slove (QVariant  value){mSlotCallBack->onSlovedCalled(value);}
    virtual void reject(QVariant value){mSlotCallBack->onRejectedCalled(value);}

private:
    struct SlotCallback
    {
        ObserveeSlot    onObserveeCalled;
        OnSlovedSlot    onSlovedCalled;
        OnRejectedSlot  onRejectedCalled;
    };

    SlotCallback* mSlotCallBack;
};


class DeferredHandler : public IDeferred
{
public:
    DeferredHandler()
    {

        DeferredHandler* self   = this;
        qDebug("@create DeferredHandler =%p", this);

        mSignalHandler   = new SignalsHandler();
        mSlotsHandler    = new SlotsHandler();

        mSlotsHandler->bindObservee([self](){
            self->onObeserveeCalling();
        })
       .bindOnSloved([self](QVariant value){
            self->onSlovedCalling(value);
        })
       .bindOnRejected([self](QVariant value){
            self->onRejectedCalling(value);
        });

        QObject::connect(mSignalHandler, SIGNAL(sigCallObservee()),        mSlotsHandler, SLOT(callObservee()),   Qt::QueuedConnection);
        QObject::connect(mSignalHandler, SIGNAL(sigSloved(QVariant )),     mSlotsHandler, SLOT(slove(QVariant )), Qt::QueuedConnection);
        QObject::connect(mSignalHandler, SIGNAL(sigRejected(QVariant )),   mSlotsHandler, SLOT(reject(QVariant )),Qt::QueuedConnection);
        QObject::connect(mSignalHandler, SIGNAL(sigDestoryed()),           mSlotsHandler, SLOT(deleteLater()));
    }

    virtual ~DeferredHandler(){
        qDebug("@Destory DeferredHandler =%p", this);

        if(mSignalHandler != nullptr) mSignalHandler->release();

        mSignalHandler = nullptr;
    }


    void slove(QVariant   value)
    {
        mSignalHandler->slove(value);
    }

    void reject(QVariant value)
    {
        mSignalHandler->reject(value);
    }

    void callObsevee(){
        mSignalHandler->callObservee();
    }

    DeferredHandler(const DeferredHandler& other)=delete;
    DeferredHandler& operator() (const DeferredHandler& other)=delete;

protected:
//    virtual void onTaskFinishedDone(DeferredHandler* self){}
    void            release(){delete this;}

    virtual void    onSlovedCalling(QVariant& value)=0;
    virtual void    onRejectedCalling(QVariant& value)=0;
    virtual void    onObeserveeCalling()=0;

private:
    SignalsHandler*     mSignalHandler;
    SlotsHandler*       mSlotsHandler;

};


}


template<typename SlovedArgType, typename RejectedArgType>
class DeferredTask
{
public:

    class Deferred;

    using _Arg_on_Slove  = typename std::decay<SlovedArgType>::type;
    using _Arg_on_Reject = typename std::decay<RejectedArgType>::type;

    typedef std::function<void(Deferred& )>                 Observee;
    typedef std::function<void(Deferred& , _Arg_on_Slove)>  OnSloved;
    typedef std::function<void(_Arg_on_Reject)>             OnRejected;


    class Deferred : public DeferredTaskPrivate::DeferredHandler
    {
    public:
          Deferred(Observee          observee,
                   DeferredTask*     taskMananger)
              : DeferredTaskPrivate::DeferredHandler()
              , mObseveeCallback(std::move(observee))
              , mDeferredQueueMananger(taskMananger)
          {

          }

          ~Deferred()
          {

          }

          Deferred& then(OnSloved _onSlove) {
              mSlovedQueue.enqueue(std::move(_onSlove));
              return *this;
          }

          Deferred& catching(OnRejected f) {
              mOnRejectCallback = std::move(f);
              return *this;
          }

          Deferred& start()
          {
              callObsevee();
              return *this;
          }
    protected:
          virtual void onTaskFinishedDone(DeferredTaskPrivate::DeferredHandler* self)
          {
              qDebug()<<"onTaskFinishedDone Deferred= "<<this;
              release();
          }

          virtual void    onSlovedCalling(QVariant& value)
          {
              // if is empty.
              if(mSlovedQueue.isEmpty()){
                  onTaskFinishedDone(this);
                  return;
              }
              // call on sloved.
              _onSlovedcallbackCalling(value);

            /*
                fixed-crash:
                    it was crashing,when the event came from the other object,
                    but the defer object released in here,due to the
                    _onSlovedcallbackCalling was binding a none blocking function,

            */
              // if this is last one.
//              if(mSlovedQueue.isEmpty()){
//                  onTaskFinishedDone(this);
//                  return;
//              }
          }

          virtual void onRejectedCalling(QVariant& value)
          {
              if(!mOnRejectCallback) return;

              mOnRejectCallback(value.value<_Arg_on_Reject>());
              onTaskFinishedDone(this);
          }

          virtual void onObeserveeCalling()
          {
             mObseveeCallback(*this);
          }

          void _onSlovedcallbackCalling(QVariant& value){
              //qDebug()<<"before sloved calling remain in queue "<<mSlovedQueue.size();
              OnSloved slovedRoutine= std::move(mSlovedQueue.dequeue());
              slovedRoutine(*this, value.value<_Arg_on_Slove>());
              //qDebug()<<"after sloved calling remain in queue "<<mSlovedQueue.size();
          }
    private:
          DeferredTask*     mDeferredQueueMananger;

          Observee          mObseveeCallback;
          QQueue<OnSloved>  mSlovedQueue;
          OnRejected        mOnRejectCallback;
    };


    DeferredTask()
    {

    }

    virtual ~DeferredTask()
    {
        qDebug()<<"~DeferredTask()";
    }

    Deferred& observeOn(Observee observee) {
        // no need putting a mutex in deferred object due to we assumed
        // the deferred object was not running in the concurrent enviroment.

        QMutexLocker _auto_locker(&mMutex);
        Deferred* def = new Deferred(observee, this);
        return *def;
    }

private:
    QMutex               mMutex;
    friend class DeferredTask::Deferred;
};


#endif // DEFERREDTASK_H
