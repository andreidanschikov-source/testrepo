#include <atomic>
#include <cstdint>
#include <utility>

template <typename T>
class LockFreeStack
{
public:
    void push(T value)
    {
        auto t = free_.load(std::memory_order_acquire);
        //pop from free list
        while (t.node && !free_.compare_exchange_weak(t
                                            , Tagged{t.node->next, t.tag + 1}
                                            , std::memory_order_acq_rel
                                            , std::memory_order_acquire));

        //set data on node
        if (!t.node)
        {
            t.node = new Node{std::move(value), nullptr};
            
        }
        else
        {
            t.node->value = std::move(value);
            t.node->next = nullptr;
        }

        //push
        for (;;)
        {
            auto tmp = head_.load(std::memory_order_acquire);
            t.node->next = tmp.node;
            if (head_.compare_exchange_strong(tmp
                        , Tagged{t.node, tmp.tag + 1}
                        , std::memory_order_release
                        , std::memory_order_relaxed))
            {
                break;
            }
        }
    }

    bool pop(T& out) // false if empty
    {  
        auto t = head_.load(std::memory_order_acquire);
        //pop
        while (t.node && !head_.compare_exchange_weak(t
                  , Tagged{t.node->next, t.tag + 1}
                  , std::memory_order_acq_rel
                  , std::memory_order_acquire));

        if (!t.node)
        {
            return false;
        }

        //get data from node
        out = std::move(t.node->value);
        t.node->next = nullptr;
        
        //push to free list
        for (;;)
        {
            auto tmp = free_.load(std::memory_order_acquire);
            t.node->next = tmp.node;
            if (free_.compare_exchange_strong(tmp
                        , Tagged{t.node, tmp.tag + 1}
                        , std::memory_order_release
                        , std::memory_order_relaxed))
            {
                break;
            }
        }
        
        return true;
    }

private:
    struct Node { T value; Node* next; };
    struct alignas(2 * sizeof(void*)) Tagged { Node* node; std::uintptr_t tag; };

    std::atomic<Tagged> head_ = Tagged{nullptr, 0};
    std::atomic<Tagged> free_ = Tagged{nullptr, 0};

    static_assert(std::atomic<Tagged>::is_always_lock_free);
};

int main() {
    LockFreeStack<int> s;

    s.push(5);
    int a = 0;
    return s.pop(a) ? a : -1;
}
