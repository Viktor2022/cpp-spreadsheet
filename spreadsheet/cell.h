#pragma once

#include "common.h"
#include "formula.h"
#include <set>
#include <optional>

class Impl
{
public:
    /**
     * \brief Значение ячейки
     * 
     * \return  
     */
    virtual CellInterface::Value GetValue() const = 0;
    virtual bool IsEmpty() const = 0;
    /**
     * \brief Сырое содержимое ячейки
     * 
     * \return  
     */
    virtual std::string GetText() const = 0;

    virtual std::vector<Position> GetReferencedCells() const = 0;

    virtual ~Impl() = default;
};

class Cell : public CellInterface {
public:
    Cell(SheetInterface &sheet);
    ~Cell();

    void Clear();
    void Set(Position pos, std::string text);

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    std::unique_ptr<Impl> impl_;
    SheetInterface &sheet_;

    // Ячейки, которые зависят от этой ячейки
    std::set<Position> dependenceCells_;

    mutable std::optional<CellInterface::Value> cache_;
    
    /**
     * \brief Удаление кеша у других ячеек, которые зависят от этой.
     * 
     */
    void InvalidateDependentsCache();
    
    /**
     * \brief Удаление зависимости другой ячейки от этой ячейки
     * 
     */
    void InvalidateReferencedCells();

    /**
     * \brief Установка зависимости от другой ячейки
     * \param dependencePos - позиция ячейки, от которой зависит
     * 
     */
    void SetReferencedCells(Position dependencePos);

    void InvalidateCache();

    void SetCache(const CellInterface::Value &value);
};