#ifndef DEFERREDTASK_HPP
#define DEFERREDTASK_HPP

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QRunnable>
#include <QVector>
#include <QVariant>
#include <QMutex>

#include <functional>


namespace DeferredTaskPrivate {
class DeferredInterface
{
public:
    virtual void slove(QVariant  value)  =0;
    virtual void reject(QVariant value)  =0;
};


class ObServeeInterface
{
public:
    virtual ~ObServeeInterface(){}

    virtual void callObservee(DeferredInterface& ) =0;
    virtual void slove(QVariant  value)  =0;
    virtual void reject(QVariant value)  =0;
};




class BaseDeferred: public QObject, virtual public DeferredInterface
{
    Q_OBJECT
public :

    virtual ~BaseDeferred()
    {
        if(mObservee) delete mObservee;
        LOGGING_LEVEL("debug")<<"@destory BaseDeferred: "<< this;
    }

    virtual void slove (QVariant  value){emit sigSloved(value);}
    virtual void reject(QVariant value){emit sigRejected(value);}

    void callObservee(){emit sigCallObservee();}

    void release(){ deleteLater();}

    BaseDeferred(ObServeeInterface* observee,QObject* parent=nullptr)
            :QObject(parent)
            ,mObservee(observee)
    {
        connect(this, SIGNAL(sigCallObservee()), this, SLOT(onCallObservee()), Qt::QueuedConnection);
        connect(this, SIGNAL(sigSloved(QVariant )),           this, SLOT(onCallSlove(QVariant )),        Qt::QueuedConnection);
        connect(this, SIGNAL(sigRejected(QVariant )),         this, SLOT(onCallReject(QVariant )),       Qt::QueuedConnection);
    }

signals:
    void sigSloved(QVariant );
    void sigRejected(QVariant );
    void sigCallObservee();


private slots:

    void onCallObservee()
    {
        mObservee->callObservee(*this);
    }

    void onCallSlove(QVariant  value) {
        mObservee->slove(value);
        this->release(); //release self
    }

    void onCallReject(QVariant value) {
        mObservee->reject(value);
        this->release(); //release self
    }

protected:
    ObServeeInterface*  mObservee;
};

}


template<typename T>
class DeferredTask
{
public:
    using Arg_Type = typename std::decay<T>::type;

    class Deferred;

    typedef std::function<void(Deferred& )>            Observee;
    typedef std::function<void(Arg_Type )>             EventHandler;
    typedef std::function<void(void)>                  OnFinishAll;

    typedef EventHandler                          OnSloved;
    typedef EventHandler                          OnRejected;

    class ObServeeWrapper : public virtual DeferredTaskPrivate::ObServeeInterface
    {
    public:

        ObServeeWrapper(DeferredTask* taskmanager,Observee observee):mTaskManager(taskmanager),mObservee(observee){}

        void bindOnSlove(OnSloved f){mOnSloved      = f;}
        void bindOnReject(OnRejected f){mOnReject   = f;}

        virtual void callObservee(DeferredTaskPrivate::DeferredInterface& deferred){mObservee(dynamic_cast<Deferred&>(deferred));}

        virtual void slove(QVariant  value){

                if(mOnSloved) mOnSloved(value.value<Arg_Type>());

                if(mTaskManager) mTaskManager->disposeNext();
        }

        virtual void reject(QVariant value){
                if(mOnReject) mOnReject(value.value<Arg_Type>());

                if(mTaskManager) mTaskManager->disposeNext();
        }

    private:
        Observee        mObservee;
        OnSloved        mOnSloved;
        OnRejected      mOnReject;
        DeferredTask*   mTaskManager;
    };

    class Deferred : public DeferredTaskPrivate::BaseDeferred
    {
    public:
        Deferred(ObServeeWrapper* observee):BaseDeferred(observee, 0){mDuplicate=(ObServeeWrapper*)(observee);}
        ObServeeWrapper* getObservee(){return mDuplicate;}

    private:
        ObServeeWrapper*    mDuplicate;
    };


    static inline Deferred* __create_deferred(DeferredTask* self,Observee observee)
    {
        return new Deferred(new ObServeeWrapper(self, observee));
    }

    DeferredTask()
    {

    }

    virtual ~DeferredTask()
    {

    }

    DeferredTask& ObserveeOn(Observee observee)
    {
        mDeferredTaskQueue.append(__create_deferred(this,observee));
        return *this;
    }

    DeferredTask& then(Observee observee) {return ObserveeOn(observee);}

    DeferredTask& onSlove(OnSloved  f){
        ObServeeWrapper* _d = mDeferredTaskQueue.last()->getObservee();
        _d->bindOnSlove(f);
        return *this;
    }

    DeferredTask& onRejected(OnRejected f){
        ObServeeWrapper* _d = mDeferredTaskQueue.last()->getObservee();
        _d->bindOnReject(f);
        return *this;
    }

//    DeferredTask& onFinishAll(OnFinishAll onFinished){

//        return *this;
//    }

    DeferredTask& start()
    {
        disposeNext();
        return *this;
    }

private:
    void disposeNext()
    {
        LOGGING_LEVEL("debug")<<"mDeferredTaskQueue.isEmpty() "<<mDeferredTaskQueue.isEmpty();

        if(mDeferredTaskQueue.isEmpty()) return;

        Deferred* task = mDeferredTaskQueue.front();
        task->callObservee();
        mDeferredTaskQueue.removeFirst();
    }


private:
    QList<Deferred*>        mDeferredTaskQueue;
};


#endif // DEFERREDTASK_HPP
