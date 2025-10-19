#include "order_book.hpp"
#include <iostream>
#include <iomanip>

int main()
{
    try {
        OrderBook book;

        std::cout << "=== Limit Order Book Demo ===\n\n";

        std::cout << "Creating buy orders...\n";
        uint64_t buy1 = book.createOrder(OrderType::BUY, 99.50, 100);
        std::cout << "Buy order: ID = " << buy1 << " @ $99.50 x 100\n";

        uint64_t buy2 = book.createOrder(OrderType::BUY, 99.50, 50);
        std::cout << "Buy order: ID = " << buy2 << " @ $99.50 x 50\n";

        uint64_t buy3 = book.createOrder(OrderType::BUY, 99.25, 200);
        std::cout << "Buy order: ID = " << buy3 << " @ $99.25 x 200\n";

        uint64_t buy4 = book.createOrder(OrderType::BUY, 99.00, 150);
        std::cout << "Buy order: ID = " << buy4 << " @ $99.00 x 150\n\n";

        std::cout << "Creating sell orders...\n";
        uint64_t sell1 = book.createOrder(OrderType::SELL, 100.50, 100);
        std::cout << "Sell order: ID = " << sell1 << " @ $100.50 x 100\n";

        uint64_t sell2 = book.createOrder(OrderType::SELL, 100.50, 75);
        std::cout << "Sell order: ID = " << sell2 << " @ $100.50 x 75\n";

        uint64_t sell3 = book.createOrder(OrderType::SELL, 100.75, 250);
        std::cout << "Sell order: ID = " << sell3 << " @ $100.75 x 250\n";

        uint64_t sell4 = book.createOrder(OrderType::SELL, 101.00, 120);
        std::cout << "Sell order: ID = " << sell4 << " @ $101.00 x 120\n\n";

        book.printBook(5);

        auto [bid, ask] = book.getBestBidAsk();
        std::cout << "Best Bid: $" << std::fixed << std::setprecision(2) << bid << "\n";
        std::cout << "Best Ask: $" << std::fixed << std::setprecision(2) << ask << "\n\n";

        std::cout << "--- Cancelling order " << buy2 << " ---\n";
        if (book.cancelOrder(buy2)) {
            std::cout << "Order cancelled successfully\n\n";
        }

        book.printBook(5);

        std::cout << "--- Updating order " << buy3 << " to $98.75 ---\n";
        if (book.updateOrder(buy3, 98.75)) {
            std::cout << "Order updated successfully\n\n";
        }

        book.printBook(5);

        std::cout << "--- Depth Snapshot (Top 3) ---\n";
        std::vector<PriceLevel> bids, asks;
        book.getDepthSnapshot(3, bids, asks);

        std::cout << "Bids:\n";
        for (const auto& level : bids) {
            std::cout << "  $" << std::fixed << std::setprecision(2) 
                      << level.price << " x " << level.total_quantity << "\n";
        }

        std::cout << "Asks:\n";
        for (const auto& level : asks) {
            std::cout << "  $" << std::fixed << std::setprecision(2) 
                      << level.price << " x " << level.total_quantity << "\n";
        }

        std::cout << "\nTotal orders: " << book.getOrderCount() << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
