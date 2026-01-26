#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;
using namespace std;

void Sheet::UpdateSize(Position pos)
{
    if (pos.row >= static_cast<int>(table_.size()))
    {
        table_.resize(pos.row + 1);
    }
    if (pos.col >= static_cast<int>(table_[pos.row].size()))
    {
        table_[pos.row].resize(pos.col + 1);
    }
}

void Sheet::RecalculatePrintableSize()
{
    int maxRow = 0;
    int maxCol = 0;

    for (int r = 0; r < static_cast<int>(table_.size()); r++)
    {
        for (int c = 0; c < static_cast<int>(table_[r].size()); c++)
        {
            if (table_[r][c])
            {
                maxRow = max(maxRow, r + 1);
                maxCol = max(maxCol, c + 1);
            }
        }
    }
    printableSize_ = {maxRow, maxCol};
}

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position");
    }

    // Это формула
    if (!text.empty() && text[0] == '=' && text.size() > 1) {
        std::unique_ptr<FormulaInterface> newFormula;
        try 
        {
            newFormula = ParseFormula(text.substr(1));
        }
        catch(const FormulaException&)
        {
            throw;
        }

        // Проверить на обращение к ячейке, которой еще нет
        for (auto &pos : newFormula->GetReferencedCells())
        {
            if (!GetCell(pos))
            {
                UpdateSize(pos);
                std::unique_ptr<Cell> &ptr = GetPtr(pos);
                ptr = make_unique<Cell>(*this);
                printableSize_.rows = max(printableSize_.rows, pos.row + 1);
                printableSize_.cols = max(printableSize_.cols, pos.col + 1);
            }
        }

        std::set<Position> visiting, visited;
        // тут остановился
        if (HasCircularDependency(pos, newFormula->GetReferencedCells(), visiting, visited, pos))
        {
            throw CircularDependencyException("");
        }
    }

    UpdateSize(pos);

    std::unique_ptr<Cell> &ptr = GetPtr(pos);

    if (!ptr)
    {
        ptr = make_unique<Cell>(*this);
    }
    ptr->Set(pos, text);

    printableSize_.rows = max(printableSize_.rows, pos.row + 1);
    printableSize_.cols = max(printableSize_.cols, pos.col + 1);
}

const CellInterface* Sheet::GetCell(Position pos) const
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position");
    }

    if (pos.row >= static_cast<int>(table_.size()) ||
        pos.col >= static_cast<int>(table_[pos.row].size()))
    {
        return nullptr;
    }

    return GetPtr(pos).get();
}

CellInterface* Sheet::GetCell(Position pos)
{
    return const_cast<CellInterface*>(static_cast<const Sheet*>(this)->GetCell(pos));
}


void Sheet::ClearCell(Position pos)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position");
    }
    if (pos.row >= static_cast<int>(table_.size()) ||
        pos.col >= static_cast<int>(table_[pos.row].size()))
        {
            return;
        }
    
    table_[pos.row][pos.col].reset();
    RecalculatePrintableSize();
}

Size Sheet::GetPrintableSize() const
{
    return printableSize_;
}

void Sheet::PrintValues(std::ostream& output) const
{
    for (int r = 0; r < printableSize_.rows; r++)
    {
        for (int c = 0; c < printableSize_.cols; c++)
        {
            if (c > 0)
            {
                output << '\t';
            }
            if (r >= static_cast<int>(table_.size()) ||
                c >= static_cast<int>(table_[r].size()))
            {
                continue;
            }
            if (auto cell = GetCell({r, c}))
            {
                visit([&output](const auto &val)
                {
                    output << val;
                }, cell->GetValue());
            }
        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const
{
    for (int r = 0; r < printableSize_.rows; r++)
    {
        for (int c = 0; c < printableSize_.cols; c++)
        {
            if (c > 0)
            {
                output << '\t';
            }
            if (r >= static_cast<int>(table_.size()) ||
                c >= static_cast<int>(table_[r].size()))
            {
                continue;
            }
            if (auto cell = GetCell({r, c}))
            {
                output << cell->GetText();
            }
        }
        output << '\n';
    }
}

bool Sheet::HasCircularDependency(
    Position current,
    const std::vector<Position>& start_refs,
    std::set<Position>& visiting,
    std::set<Position>& visited,
    Position start_cell
) const
{
    if (visiting.count(current)) return true;  // цикл
    if (visited.count(current)) return false;  // уже проверено

    visiting.insert(current);

    std::vector<Position> refs;

    if (current == start_cell)
    {
        refs = start_refs;  // новые ссылки для стартовой ячейки
    }
    else
    {
        if (const CellInterface* cell = GetCell(current)) 
        {
            refs = cell->GetReferencedCells();
        }
    }

    for (const Position& ref : refs) {
        if (HasCircularDependency(ref, start_refs, visiting, visited, start_cell)) {
            return true;
        }
    }

    visiting.erase(current);
    visited.insert(current);
    return false;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}