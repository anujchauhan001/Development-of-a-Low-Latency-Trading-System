#include <iostream>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <chrono>

// Enum to define the type of order (Buy/Sell)
enum OrderType { BUY, SELL };
struct Order {
    int id;                   // Unique identifier for the order
    OrderType type;           // Order type: BUY or SELL
    double price;             // Order price
    int quantity;             // Order quantity
    long timestamp;           // Timestamp of order creation

    Order(int id, OrderType type, double price, int quantity, long timestamp)
        : id(id), type(type), price(price), quantity(quantity), timestamp(timestamp) {}
};

class OrderBook {
private:
    std::map<double, std::queue<Order>> buyOrders; 
    std::map<double, std::queue<Order>> sellOrders; 
    std::mutex mtx; 

public:
    void addOrder(const Order& order) {
        std::lock_guard<std::mutex> lock(mtx); 
        if (order.type == BUY) {
            buyOrders[order.price].push(order); 
        } else {
            sellOrders[order.price].push(order); 
        }
    }

    std::pair<Order, Order> matchOrder() {
        std::lock_guard<std::mutex> lock(mtx); 
        if (!buyOrders.empty() && !sellOrders.empty()) {
            auto highestBuy = buyOrders.rbegin(); 
            auto lowestSell = sellOrders.begin(); 

            if (highestBuy->first >= lowestSell->first) {
                Order buyOrder = highestBuy->second.front();
                Order sellOrder = lowestSell->second.front();
                highestBuy->second.pop();
                lowestSell->second.pop();

                if (highestBuy->second.empty()) buyOrders.erase(highestBuy->first);
                if (lowestSell->second.empty()) sellOrders.erase(lowestSell->first);

                return {buyOrder, sellOrder}; 
            }
        }
        return {Order(-1, BUY, 0, 0, 0), Order(-1, SELL, 0, 0, 0)};
    }
};

class MatchingEngine {
private:
    OrderBook orderBook; 

public:
    void receiveOrder(const Order& order) {
        orderBook.addOrder(order); 
    }

    void processOrders() {
        while (true) {
            auto matchedOrders = orderBook.matchOrder(); 
            if (matchedOrders.first.id != -1 && matchedOrders.second.id != -1) {
                executeTrade(matchedOrders.first, matchedOrders.second); 
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); 
        }
    }

    void executeTrade(const Order& buyOrder, const Order& sellOrder) {
        int tradeQuantity = std::min(buyOrder.quantity, sellOrder.quantity);
        std::cout << "Trade executed: Buy Order ID " << buyOrder.id << " matched with Sell Order ID " << sellOrder.id
                  << " for quantity " << tradeQuantity << " at price " << sellOrder.price << "\n";
    }
};

int main() {
    MatchingEngine engine; 

    Order order1(1, BUY, 100.5, 10, 1628908800000);  
    Order order2(2, SELL, 100.5, 5, 1628908801000);  
    Order order3(3, SELL, 100.0, 5, 1628908802000);  

    std::thread processingThread(&MatchingEngine::processOrders, &engine);

    engine.receiveOrder(order1);
    engine.receiveOrder(order2);
    engine.receiveOrder(order3);
    processingThread.join();
    return 0;
}
