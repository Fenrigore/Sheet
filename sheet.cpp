#include "sheet.h"
#include <iostream>
#include <algorithm>

using namespace std::literals;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) throw InvalidPositionException("Invalid position");

    auto it = cells_.find(pos);
    if (it == cells_.end()) {
        // Создаем новую ячейку
        cells_[pos] = std::make_unique<Cell>(*this, pos);
    }
    cells_[pos]->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) throw InvalidPositionException("Invalid position");
    auto it = cells_.find(pos);
    // Проверяем, существует ли ячейка и не пустая ли она
    if (it != cells_.end() && !it->second->GetText().empty()) {
        return it->second.get();
    }
    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) throw InvalidPositionException("Invalid position");
    auto it = cells_.find(pos);
    // Аналогичная проверка
    if (it != cells_.end() && !it->second->GetText().empty()) {
        return it->second.get();
    }
    return nullptr;
}

Cell* Sheet::GetConcreteCell(Position pos) {
    if (!pos.IsValid()) throw InvalidPositionException("Invalid position");
    auto it = cells_.find(pos);
    if (it != cells_.end()) return it->second.get();
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) throw InvalidPositionException("Invalid position");
    auto it = cells_.find(pos);
    if (it != cells_.end()) {
        it->second->Clear();
        // Мы не удаляем пустую ячейку из словаря, чтобы не разорвать граф связей,
        // метод GetPrintableSize() всё равно ее проигнорирует.
    }
}

Size Sheet::GetPrintableSize() const {
    Size size{ 0, 0 };
    for (const auto& [pos, cell] : cells_) {
        if (cell && !cell->GetText().empty()) {
            size.rows = std::max(size.rows, pos.row + 1);
            size.cols = std::max(size.cols, pos.col + 1);
        }
    }
    return size;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int r = 0; r < size.rows; ++r) {
        for (int c = 0; c < size.cols; ++c) {
            if (c > 0) output << '\t';
            const auto* cell = GetCell({ r, c });
            if (cell) {
                std::visit([&output](const auto& val) { output << val; }, cell->GetValue());
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int r = 0; r < size.rows; ++r) {
        for (int c = 0; c < size.cols; ++c) {
            if (c > 0) output << '\t';
            const auto* cell = GetCell({ r, c });
            if (cell) {
                output << cell->GetText();
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}