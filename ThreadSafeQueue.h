/**
 * @file ThreadSafeQueue.h
 *
 * @brief ThreadSafeQueue class is an implementation of thread-safe queue adapter with strong exception safe guarantee.
 *
 * @author Hovsep Papoyan
 * Contact: papoyanhovsep93@gmail.com
 * @Date 2024-04-12
 *
 */

#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

namespace mt
{
    template<typename AdaptElem,
        template<typename, typename> typename Cont = std::deque, typename ContElem = AdaptElem,
        template<typename> typename Alloc = std::allocator, typename AllocElem = ContElem>
    class ThreadSafeQueue
    {
        std::queue<std::shared_ptr<AdaptElem>,
            Cont<std::shared_ptr<ContElem>,
            Alloc<std::shared_ptr<AllocElem>>>> m_queue;
        mutable std::mutex m_mutex;
        std::condition_variable m_condVar;

    public:
        ThreadSafeQueue() = default;
        ~ThreadSafeQueue() = default;
        ThreadSafeQueue(const ThreadSafeQueue& rhs);
        ThreadSafeQueue(ThreadSafeQueue&& rhs);
        ThreadSafeQueue& operator=(const ThreadSafeQueue& rhs);
        ThreadSafeQueue& operator=(ThreadSafeQueue&& rhs);
        void swap(ThreadSafeQueue& rhs);

        void push(AdaptElem elem);
        void waitAndPop(AdaptElem& elem);
        std::shared_ptr<AdaptElem> waitAndPop();
        bool tryPop(AdaptElem& elem);
        std::shared_ptr<AdaptElem> tryPop();
        std::shared_ptr<AdaptElem> pop();
        void pop(AdaptElem& elem);
    };

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>::ThreadSafeQueue(const ThreadSafeQueue& rhs)
    {
        std::lock_guard<std::mutex> lock(rhs.m_mutex);
        m_queue = rhs.m_queue;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>::ThreadSafeQueue(ThreadSafeQueue&& rhs)
    {
        std::lock_guard<std::mutex> lock(rhs.m_mutex);
        m_queue = std::move(rhs.m_queue);
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>&
        ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>::operator=(const ThreadSafeQueue& rhs)
    {
        if (this != &rhs)
        {
            std::scoped_lock lock(m_mutex, rhs.m_mutex);
            m_queue = rhs.m_queue;
        }
        return *this;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>&
        ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>::operator=(ThreadSafeQueue&& rhs)
    {
        if (this != &rhs)
        {
            std::scoped_lock lock(m_mutex, rhs.m_mutex);
            m_queue = std::move(rhs.m_queue);
        }
        return *this;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    void
        ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>::swap(ThreadSafeQueue& rhs)
    {
        if (this != &rhs)
        {
            std::scoped_lock lock(m_mutex, rhs.m_mutex);
            std::swap(m_queue, rhs.m_queue);
        }
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    void
        ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>::push(AdaptElem elem)
    {
        std::shared_ptr<AdaptElem> data(std::make_shared<AdaptElem>(std::move(elem)));
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(data));
        }
        m_condVar.notify_one();
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    void
        ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>::waitAndPop(AdaptElem& elem)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condVar.wait(lock, [this] { return !m_queue.empty(); });
        elem = std::move(*m_queue.front());
        m_queue.pop();
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    std::shared_ptr<AdaptElem>
        ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>::waitAndPop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condVar.wait(lock, [this] { return !m_queue.empty(); });
        std::shared_ptr<AdaptElem> res = std::move(m_queue.front());
        m_queue.pop();
        return res;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    bool
        ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>::tryPop(AdaptElem& elem)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
        {
            return false;
        }
        elem = std::move(*m_queue.front());
        m_queue.pop();
        return true;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    std::shared_ptr<AdaptElem>
        ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>::tryPop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
        {
            return std::shared_ptr<AdaptElem>{};
        }
        std::shared_ptr<AdaptElem> res = std::move(m_queue.front());
        m_queue.pop();
        return res;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    std::shared_ptr<AdaptElem>
        ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>::pop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
        {
            throw std::runtime_error("The ThreadSafeQueue is empty");
        }
        std::shared_ptr<AdaptElem> res = std::move(m_queue.front());
        m_queue.pop();
        return res;
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    void
        ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>::pop(AdaptElem& elem)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
        {
            throw std::runtime_error("The ThreadSafeQueue is empty");
        }
        elem = std::move(*m_queue.front());
        m_queue.pop();
    }

    template<typename AdaptElem,
        template<typename, typename> typename Cont, typename ContElem,
        template<typename> typename Alloc, typename AllocElem>
    void
        swap(ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>& lhs, ThreadSafeQueue<AdaptElem, Cont, ContElem, Alloc, AllocElem>& rhs)
    {
        lhs.swap(rhs);
    }
} // namespace mt

#endif
