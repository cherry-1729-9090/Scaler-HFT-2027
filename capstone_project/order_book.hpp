#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <unordered_map>
#include <stdexcept>
#include <chrono>
#include <iostream>
#include <iomanip>

enum class OrderType : uint8_t {
    BUY = 0,
    SELL = 1
};

struct Order {
    uint64_t order_id;
    OrderType order_type;
    double price;
    uint64_t quantity;
    uint64_t timestamp_ns;

    Order(uint64_t id, OrderType type, double p, uint64_t qty, uint64_t ts)
        : order_id(id), order_type(type), price(p), quantity(qty), timestamp_ns(ts)
    {
    }

    Order() 
        : order_id(0), order_type(OrderType::BUY), price(0.0), quantity(0), timestamp_ns(0)
    {
    }
};

struct PriceLevel {
    double price;
    uint64_t total_quantity;

    PriceLevel(double p = 0.0, uint64_t qty = 0)
        : price(p), total_quantity(qty)
    {
    }
};

class OrderBook {
private:
    struct OrderLocation {
        double price;
        size_t index;
        bool is_buy;
    };

    std::map<double, std::vector<Order>, std::greater<double>> bid_side;
    std::map<double, std::vector<Order>, std::less<double>> ask_side;
    std::unordered_map<uint64_t, OrderLocation> order_lookup;

    uint64_t total_orders;
    uint64_t order_id_counter;
    double best_bid_price;
    double best_ask_price;
    uint64_t best_bid_qty;
    uint64_t best_ask_qty;

    void update_best_prices();
    void invalidate_best_prices();

public:
    OrderBook();
    ~OrderBook() = default;

    uint64_t createOrder(OrderType order_type, double price, uint64_t quantity);
    bool cancelOrder(uint64_t order_id);
    bool updateOrder(uint64_t order_id, double new_price);

    void getDepthSnapshot(size_t depth, std::vector<PriceLevel>& bids, 
                          std::vector<PriceLevel>& asks) const;
    
    std::pair<double, double> getBestBidAsk() const;
    std::pair<uint64_t, uint64_t> getBestBidAskQuantities() const;

    void printBook(size_t depth = 10) const;
    uint64_t getOrderCount() const { return total_orders; }
    bool orderExists(uint64_t order_id) const;
};
