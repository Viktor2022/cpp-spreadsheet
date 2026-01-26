#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

/**
 * \brief Пустая ячейка
 * 
 */
class EmptyImpl : public Impl
{
public:
    EmptyImpl() = default;

    bool IsEmpty() const override
    {
        return true;
    }

    CellInterface::Value GetValue() const override
    {
        return CellInterface::Value("");
    }
    std::string GetText() const override
    {
        return "";
    }

    std::vector<Position> GetReferencedCells() const override
    {
        return {};
    }

    ~EmptyImpl() = default;
};

/**
 * \brief Текстовая ячейка
 * 
 */
class TextImpl : public Impl
{
public:
    TextImpl(std::string text) : text_(std::move(text))
    {}

    bool IsEmpty() const override
    {
        return text_.empty();
    }

    CellInterface::Value GetValue() const override
    {
        if (text_[0] == '\'')
        {
            return CellInterface::Value(text_.substr(1));
        }
        else
        {
            return CellInterface::Value(text_);
        }
    }
    std::string GetText() const override
    {
        return text_;
    }

    std::vector<Position> GetReferencedCells() const override
    {
        return {};
    }

    ~TextImpl() = default;
private:
    std::string text_;
};

/**
 * \brief Формульная ячейка
 * 
 */
class FormulaImpl : public Impl
{
public:
    FormulaImpl(std::string formula, SheetInterface &sheet) :   formula_(ParseFormula(std::move(formula))),
                                                                sheet_(sheet)
    {}

    bool IsEmpty() const override
    {
        return false;
    }

    CellInterface::Value GetValue() const override
    {
        return ConvertToValue(formula_->Evaluate(sheet_));
    }
    std::string GetText() const override
    {
        return "=" + formula_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const override
    {
        return formula_->GetReferencedCells();
    }

    ~FormulaImpl() = default;
private:

    CellInterface::Value ConvertToValue(FormulaInterface::Value oldValue) const
    {
        return std::visit(
            [](auto&& arg) -> CellInterface::Value {
                return arg;  // Просто возвращаем значение
            },
            oldValue
        );
    }
    std::unique_ptr<FormulaInterface> formula_;
    SheetInterface &sheet_;
};

Cell::Cell(SheetInterface &sheet) : impl_(std::make_unique<EmptyImpl>()),
                                    sheet_(sheet)
{}

Cell::~Cell() = default;

void Cell::Set(Position pos, std::string text) 
{
    // Чистим все
    InvalidateCache();
    InvalidateDependentsCache();
    // Создаем новую ячейку и проверяем, что она вообще валидна
    if (text.empty())
    {
        impl_ = std::make_unique<EmptyImpl>();
    }
    else if (text[0] == '=' && text.size() > 1)
    {
        std::unique_ptr<FormulaImpl> tempImpl;
        try
        {
            tempImpl = std::make_unique<FormulaImpl>(std::move(text.substr(1)), sheet_);
            // Если с формулой все ок, то нужно указать остальным ячейкам, что они больше не будут для этой ячейки зависимыми
        }
        catch (FormulaException &exp)
        {
            throw;
        }
        InvalidateReferencedCells();
        impl_ = std::move(tempImpl);
        SetReferencedCells(pos);
        SetCache(GetValue());
    }
    else
    {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }
}

void Cell::Clear()
{
    Set({0, 0}, "");
}

Cell::Value Cell::GetValue() const
{
    if (!cache_) 
    {
        cache_ = impl_->GetValue();
    }
    return cache_.value();
}
std::string Cell::GetText() const
{
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const
{
    return impl_->GetReferencedCells();
}

void Cell::InvalidateDependentsCache()
{
    for (auto &pos : dependenceCells_)
    {
        if (Cell *depCell = dynamic_cast<Cell *>(sheet_.GetCell(pos)))
        {
            depCell->InvalidateCache();
            depCell->InvalidateDependentsCache();
        }
    }
}

void Cell::InvalidateReferencedCells()
{
   for(auto &pos : impl_->GetReferencedCells())
   {
        Cell *refCell = dynamic_cast<Cell *>(sheet_.GetCell(pos));
        if (!refCell)
        {
            continue;
        }
        refCell->dependenceCells_.erase(pos);
   }
}

void Cell::SetReferencedCells(Position dependencePos)
{
    for(auto &pos : impl_->GetReferencedCells())
    {
            Cell *refCell = dynamic_cast<Cell *>(sheet_.GetCell(pos));
            if (!refCell)
            {
                continue;
            }
            refCell->dependenceCells_.insert(dependencePos);
    }
}

void Cell::InvalidateCache()
{
    cache_.reset();
}

void Cell::SetCache(const CellInterface::Value &value)
{
    cache_ = value;
}
