#ifndef _CELLObjectPool_hpp_
#define _CELLObjectPool_hpp_
#include <cstdlib>
#include <mutex>
#include <assert.h>

#define _DEBUG
#ifdef _DEBUG
    #ifndef xPrintf
        #include <stdio.h>
        #define xPrintf(...) printf(__VA_ARGS__)
    #endif
#else
    #ifndef xPrintf
    #define xPrintf(...)
    #endif
#endif


template<class Type, size_t nPoolSize>
class CELLObjectPool
{
public:
    CELLObjectPool()
    {
        _pBuf = nullptr;
        initPool();
    }
    ~CELLObjectPool()
    {
        if(_pBuf)
            delete[] _pBuf;
    }
private:
    class NodeHeader
    {
    public:
        //下一块位置
        NodeHeader* pNext;
        // 内存块编号
        int nID;
        // 引用次数
        char nRef;
        //是否在内存池中
        bool bPool;
    private:
        //预留
        char c1;
        char c2;
    };
public:
    //释放对象
        void freeMemory(void *pMem)
    {
        // ::free(p)::; 调用系统路径
        // char* pDat = pMem;
        NodeHeader* pBlock = (NodeHeader*)((char*)pMem- sizeof(NodeHeader));
        assert(1 == pBlock->nRef);
        if(pBlock->bPool)
        {
            std::lock_guard<std::mutex> lg(_mutex);
            if(--pBlock->nRef != 0)
            {
                return;
            }
            pBlock->pNext = _pHeader;
            _pHeader = pBlock;
        }
        else
        {
            if(--pBlock->nRef != 0)
            {
                return;
            }
            delete[] pBlock;
        }
    }

    //申请对象内存
    void* allocMemory(size_t nSize)
    {
        NodeHeader* pReturn = nullptr;
        if(nullptr == _pHeader)
        {
            pReturn = (NodeHeader*)new char[sizeof(Type) + sizeof(NodeHeader)];
            pReturn->bPool = false;
            pReturn->nID = -1;
            pReturn->nRef = 1;
            pReturn->pNext = nullptr;

        }
        else
        {
            pReturn = _pHeader;
            _pHeader = pReturn->pNext;
            assert(0 == pReturn->nRef);
            pReturn->nRef = 1;
        }
        xPrintf("allocMem: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
        return (char*)pReturn + sizeof(NodeHeader);
    }
    //初始化对象池
    void initPool()
    {
        assert(nullptr == _pBuf);
        if(_pBuf)
            return;
        //对象池大小
        size_t realSize = sizeof(Type) +sizeof(NodeHeader);
        size_t n = nPoolSize * realSize;
        _pBuf = new char[n];
        //初始化内存池
        _pHeader = (NodeHeader*)_pBuf;
        _pHeader->bPool = true;
        _pHeader->nID = 0;
        _pHeader->nRef = 0;
        _pHeader->pNext = nullptr;

        //遍历内存块进行初始化
        NodeHeader* pTemp1 = _pHeader;
        for(size_t n=1; n<nPoolSize; n++)
        {
            NodeHeader* pTemp2 = (NodeHeader*)(_pBuf + n*realSize);
            pTemp2->bPool = true;
            pTemp2->nID = n;
            pTemp2->nRef = 0;
            pTemp2->pNext = nullptr;
            pTemp1->pNext = pTemp2;
            pTemp1 = pTemp2;
        }



    }
private:
    NodeHeader* _pHeader;
    //对象池地址
    char* _pBuf;
    std::mutex _mutex;

};

template<class Type, size_t nPoolSize>
class ObjectPoolBase
{
public:
    ObjectPoolBase()
    {

    }
    ~ObjectPoolBase()
    {

    }

    void* operator new (size_t nSize)
    {
        return objectPool().allocMemory(nSize);
    }

    void operator delete(void* p)
    {
        objectPool().freeMemory(p);
    }

    template<typename ...Args>//不定参数，可变参数
    static Type* createObject(Args ... args)
    {
        Type* obj = new Type(args...);
        return obj;   
    }

    static void destoryObject(Type* obj)
    {
        delete obj;
    }
private:
    typedef CELLObjectPool<Type, nPoolSize> ClassTypePool;
    static ClassTypePool& objectPool()
    {
        static ClassTypePool sPool;
        return sPool;
    }

};

#endif