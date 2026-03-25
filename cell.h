#pragma once

#include "common.h"
#include "formula.h"
#include <functional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet, Position pos);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    // Методы для работы с графом зависимостей
    bool IsReferenced() const;
    void InvalidateCache();
    bool HasCache() const;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    Position position_{};

    //Кэш вычислений
    mutable std::optional<Value> cache_;

    // Граф зависимостей: от кого я завишу (включая пустые ячейки)
    std::unordered_set<Cell*> referenced_cells_;
    // Граф зависимостей: кто от меня зависит
    std::unordered_set<Cell*> dependent_cells_;

    //вспомогательные методы для работы с зависимостями
    void UpdateDependencies();
    void ClearDependencies();
    void CheckCircularDependencies() const;

    Position GetPosition() const;
};