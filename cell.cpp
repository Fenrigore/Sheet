#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

class Cell::Impl {
public:
	virtual std::string GetText() const = 0;
	virtual Cell::Value GetValue() const = 0;
	virtual ~Impl() = default;
};

class Cell::EmptyImpl : public Cell::Impl {
public:
	std::string GetText() const override {
		return std::string{};
	}

	CellInterface::Value GetValue() const override {
		return std::string{};
	}
};

class Cell::TextImpl : public Cell::Impl {
public:
	TextImpl(std::string text) : text_(std::move(text)) {}

	std::string GetText() const override{
		return text_;
	}

	CellInterface::Value GetValue() const override{
		if (text_.front() == '\'') {
			return text_.size() > 1 ? text_.substr(1): "";
		}
		return text_;
	}

private:
	std::string text_;
};

struct ValueGetter {
	CellInterface::Value operator()(double value) const {
		return value;
	}

	CellInterface::Value operator()(const FormulaError& value) const {
		return value;
	}
};

class Cell::FormulaImpl : public Cell::Impl {
public:
	FormulaImpl(std::unique_ptr<FormulaInterface> formula)
		: formula_(std::move(formula)) {}

	std::string GetText() const override {
		return "=" + formula_->GetExpression();
	}

	CellInterface::Value GetValue() const override {
		return std::visit(ValueGetter{}, formula_->Evaluate(sheet_));
	}

private:
	std::unique_ptr<FormulaInterface> formula_;
};

// Реализуйте следующие методы
Cell::Cell(Sheet& sheet, Position pos): impl_(std::make_unique<EmptyImpl>())
, sheet_(sheet)
, position_(pos){}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
	if (text.empty()) {
		Clear();
	}
	else if (text.front() == '=' && text.size() > 1) {
		impl_ = std::make_unique<FormulaImpl>(ParseFormula(text.substr(1)));
	}
	else {
		impl_ = std::make_unique<TextImpl>(std::move(text));
	}
}

void Cell::Clear() {
	impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}
std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const
{
	std::vector<Position> positions{};
	for (const auto cell : referenced_cells_) {
		positions.push_back(cell->GetPosition());
	}
	return positions;
}

Position Cell::GetPosition() const {
	return position_;
}
