#include "order_book.hpp"
#include <algorithm>
#include <sstream>

OrderBook::OrderBook()
    : total_orders(0),
      order_id_counter(1),
      best_bid_price(0.0),
      best_ask_price(0.0),
      best_bid_qty(0),
      best_ask_qty(0)
{
}

uint64_t OrderBook::createOrder(OrderType order_type, double price, uint64_t quantity)
{
    if (price <= 0.0 || quantity == 0) {
        throw std::invalid_argument("Price and quantity must be positive values");
    }

    uint64_t new_order_id = order_id_counter++;
    uint64_t current_timestamp = std::chrono::system_clock::now().time_since_epoch().count();

    Order new_order(new_order_id, order_type, price, quantity, current_timestamp);

    if (order_type == OrderType::BUY) {
        bid_side[price].push_back(new_order);
        order_lookup[new_order_id] = {price, bid_side[price].size() - 1, true};
    } else {
        ask_side[price].push_back(new_order);
        order_lookup[new_order_id] = {price, ask_side[price].size() - 1, false};
    }

    total_orders++;
    invalidate_best_prices();

    return new_order_id;
}

bool OrderBook::cancelOrder(uint64_t order_id)
{
    auto lookup_it = order_lookup.find(order_id);
    if (lookup_it == order_lookup.end()) {
        return false;
    }

    const OrderLocation& location = lookup_it->second;

    std::vector<Order>* orders_at_price_ptr = nullptr;
    if (location.is_buy) {
        auto price_it = bid_side.find(location.price);
        if (price_it != bid_side.end()) {
            orders_at_price_ptr = &price_it->second;
        }
    } else {
        auto price_it = ask_side.find(location.price);
        if (price_it != ask_side.end()) {
            orders_at_price_ptr = &price_it->second;
        }
    }

    if (!orders_at_price_ptr || location.index >= orders_at_price_ptr->size()) {
        return false;
    }

    auto& orders_at_price = *orders_at_price_ptr;
    std::swap(orders_at_price[location.index], orders_at_price.back());
    orders_at_price.pop_back();

    if (location.index < orders_at_price.size()) {
        order_lookup[orders_at_price[location.index].order_id].index = location.index;
    }

    if (orders_at_price.empty()) {
        if (location.is_buy) {
            bid_side.erase(location.price);
        } else {
            ask_side.erase(location.price);
        }
    }

    order_lookup.erase(lookup_it);
    total_orders--;
    invalidate_best_prices();

    return true;
}

bool OrderBook::updateOrder(uint64_t order_id, double new_price)
{
    if (new_price <= 0.0) {
        throw std::invalid_argument("New price must be positive");
    }

    auto lookup_it = order_lookup.find(order_id);
    if (lookup_it == order_lookup.end()) {
        return false;
    }

    const OrderLocation location = lookup_it->second;

    if (location.price == new_price) {
        return true;
    }

    Order order_info;
    if (location.is_buy) {
        order_info = bid_side[location.price][location.index];
    } else {
        order_info = ask_side[location.price][location.index];
    }

    if (!cancelOrder(order_id)) {
        return false;
    }

    uint64_t ts = std::chrono::system_clock::now().time_since_epoch().count();
    Order updated_order(order_id, order_info.order_type, new_price, 
                        order_info.quantity, ts);

    if (order_info.order_type == OrderType::BUY) {
        bid_side[new_price].push_back(updated_order);
        order_lookup[order_id] = {new_price, bid_side[new_price].size() - 1, true};
    } else {
        ask_side[new_price].push_back(updated_order);
        order_lookup[order_id] = {new_price, ask_side[new_price].size() - 1, false};
    }

    invalidate_best_prices();
    return true;
}


void OrderBook::getDepthSnapshot(size_t depth, std::vector<PriceLevel>& bids, 
                                 std::vector<PriceLevel>& asks) const
{
    bids.clear();
    asks.clear();

    size_t bid_count = 0;
    for (const auto& [price, orders] : bid_side) {
        if (bid_count >= depth) {
            break;
        }

        uint64_t total_qty = 0;
        for (const auto& order : orders) {
            total_qty += order.quantity;
        }
        
        bids.emplace_back(price, total_qty);
        bid_count++;
    }

    size_t ask_count = 0;
    for (const auto& [price, orders] : ask_side) {
        if (ask_count >= depth) {
            break;
        }

        uint64_t total_qty = 0;
        for (const auto& order : orders) {
            total_qty += order.quantity;
        }
        
        asks.emplace_back(price, total_qty);
        ask_count++;
    }
}

std::pair<double, double> OrderBook::getBestBidAsk() const
{
    double best_bid = bid_side.empty() ? 0.0 : bid_side.begin()->first;
    double best_ask = ask_side.empty() ? 0.0 : ask_side.begin()->first;
    
    return {best_bid, best_ask};
}

std::pair<uint64_t, uint64_t> OrderBook::getBestBidAskQuantities() const
{
    uint64_t bid_qty = 0;
    uint64_t ask_qty = 0;

    if (!bid_side.empty()) {
        const auto& orders = bid_side.begin()->second;
        for (const auto& order : orders) {
            bid_qty += order.quantity;
        }
    }

    if (!ask_side.empty()) {
        const auto& orders = ask_side.begin()->second;
        for (const auto& order : orders) {
            ask_qty += order.quantity;
        }
    }

    return {bid_qty, ask_qty};

}


void OrderBook::printBook(size_t depth) const
{
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "                      ORDER BOOK SNAPSHOT\n";
    std::cout << std::string(70, '=') << "\n";

    std::vector<PriceLevel> bids, asks;
    getDepthSnapshot(depth, bids, asks);

    std::cout << "\n                    ASKS (Sell Orders)\n";
    std::cout << "Price              Quantity\n";
    std::cout << std::string(70, '-') << "\n";
    
    for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
        std::cout << std::fixed << std::setprecision(2) 
                  << std::setw(15) << it->price
                  << "    " << std::setw(15) << it->total_quantity << "\n";
    }

    auto [best_bid, best_ask] = getBestBidAsk();

    std::cout << "\n" << std::string(70, '-') << "\n";
    double spread = (best_ask > 0 && best_bid > 0) ? (best_ask - best_bid) : 0;
    std::cout << "SPREAD: " << std::fixed << std::setprecision(4) << spread << "\n";
    std::cout << std::string(70, '-') << "\n";

    std::cout << "\n                    BIDS (Buy Orders)\n";
    std::cout << "Price              Quantity\n";
    std::cout << std::string(70, '-') << "\n";

    for (const auto& bid : bids) {
        std::cout << std::fixed << std::setprecision(2) 
                  << std::setw(15) << bid.price
                  << "    " << std::setw(15) << bid.total_quantity << "\n";
    }

    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Total Orders in Book: " << total_orders << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

void OrderBook::update_best_prices()
{
    best_bid_price = 0.0;
    best_ask_price = 0.0;

    if (!bid_side.empty()) {
        best_bid_price = bid_side.begin()->first;
        const auto& orders = bid_side.begin()->second;
        best_bid_qty = 0;
        for (const auto& order : orders) {
            best_bid_qty += order.quantity;
        }
    }

    if (!ask_side.empty()) {
        best_ask_price = ask_side.begin()->first;
        const auto& orders = ask_side.begin()->second;
        best_ask_qty = 0;
        for (const auto& order : orders) {
            best_ask_qty += order.quantity;
        }
    }
}

void OrderBook::invalidate_best_prices()
{
    best_bid_price = 0.0;
    best_ask_price = 0.0;
    best_bid_qty = 0;
    best_ask_qty = 0;
}

bool OrderBook::orderExists(uint64_t order_id) const
{
    return order_lookup.find(order_id) != order_lookup.end();
}
