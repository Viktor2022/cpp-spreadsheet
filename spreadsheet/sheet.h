#pragma once

#include "cell.h"
#include "common.h"
#include <set>
#include <functional>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    /**
     * \brief Проверка на зацикливание
     * 
     * \param current текущая позиция
     * \param start_refs вектор проверяемых ячеек
     * \param visiting ячейки, в которые зашли
     * \param visited ячейки, которые уже проверены
     * \param start_cell исходная ячейка
     * \return  
     */
    bool HasCircularDependency(
    Position current,
    const std::vector<Position>& start_refs,
    std::set<Position>& visiting,
    std::set<Position>& visited,
    Position start_cell
    ) const;
private:
    std::unique_ptr<Cell> &GetPtr(const Position pos)
    {
        return table_[pos.row][pos.col];
    }

    const std::unique_ptr<Cell> &GetPtr(const Position pos) const
    {
        return table_[pos.row][pos.col];
    }

    void UpdateSize(Position pos);
    void RecalculatePrintableSize();

    using Row = std::vector<std::unique_ptr<Cell>>;
    std::vector<Row> table_;
    // Размер окна отображения
    Size printableSize_{0, 0};
};