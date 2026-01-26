#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;


namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression)
        try : ast_(ParseFormulaAST(std::move(expression)))  // инициализация в списке
    {
        // Проверяем все позиции на валидность сразу
        for (auto pos : ast_.GetCells())
        {
            if (!pos.IsValid())
            {
                throw FormulaException("Invalid cell reference");
            }
        }
    }
    catch (const FormulaException&)
    {
        throw;  // перебрасываем FormulaException как есть
    }
    catch (const std::exception& e)
    {
        throw FormulaException(e.what());
    }

    std::vector<Position> GetReferencedCells() const override
    {
        auto cells = ast_.GetCells();
        std::vector<Position> res(cells.begin(), cells.end());
        std::sort(res.begin(), res.end());
        res.erase(std::unique(res.begin(), res.end()), res.end());
        return res;
    }

    Value Evaluate(const SheetInterface& sheet) const override
    {
        try
        {
            const double res = ast_.Execute(sheet);
            return Value(res);
        }
        catch (const FormulaError &exp)
        {
            return Value(exp);
        }
    }
    
    std::string GetExpression() const override
    {
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