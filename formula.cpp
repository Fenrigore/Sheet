#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, const FormulaError& fe) {
    return output << "#ARITHM!";
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

    Value Evaluate() const override {
        try {
            return ast_.Execute();  // Вычисляем через FormulaAST

        }
        catch (const FormulaError& error) {
            // Ошибка вычисления (деление на 0) — возвращаем в варианте
            return error;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}