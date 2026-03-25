#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) try
    : ast_(ParseFormulaAST(std::move(expression))) {
    }catch (const FormulaException&) { //ошибка в формуле
        throw; //передаем дальше
    }
    catch (const std::exception& e) {
        throw FormulaException(e.what());
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            // Вычисляем результат как double
            double result = ast_.Execute([&sheet](Position pos) -> double {
                if (!pos.IsValid()) throw FormulaError(FormulaError::Category::Ref);
                const auto* cell = sheet.GetCell(pos);
                if (!cell) return 0.0;

                auto val = cell->GetValue();
                if (std::holds_alternative<double>(val)) {
                    return std::get<double>(val);
                }
                else if (std::holds_alternative<std::string>(val)) {
                    const std::string& str = std::get<std::string>(val);
                    if (str.empty()) return 0.0;
                    double res = 0;
                    std::istringstream in(str);
                    if (!(in >> res >> std::ws) || !in.eof()) {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                    return res;
                }
                else {
                    throw std::get<FormulaError>(val);
                }
                });

            // Проверяем результат на inf и NaN
            if (!std::isfinite(result)) {
                return FormulaError(FormulaError::Category::Arithmetic);
            }

            return result;
        }
        catch (const FormulaError& err) {
            return err;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> res;
        for (auto pos : ast_.GetCells()) {
            res.push_back(pos);
        }
        return res;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}